 // Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "26-compile.h"


static Step run_sym(Obj env, Obj code) {
  // owns env.
  assert(obj_is_sym(code));
  assert(!is(code, s_ILLEGAL)); // anything that returns s_ILLEGAL should have raised an error.
  if (code.u <= s_END_SPECIAL_SYMS.u) {
    return mk_step(env, rc_ret_val(code)); // special syms are self-evaluating.
  }
  Obj val = env_get(env, code);
  exc_check(!is(val, obj0), "lookup error: %o", code); // lookup failed.
  return mk_step(env, rc_ret(val));
}


static Step run_Quo(Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 1, "s_QUO requires 1 argument; received %i", args.len);
  return mk_step(env, rc_ret(args.els[0]));
}


static Step run_step(Obj env, Obj code);

static Step run_Do(Obj env, Mem args) {
  // owns env.
  if (!args.len) {
    return mk_step(env, rc_ret_val(s_void));
  }
  Int last = args.len - 1;
  it_mem_to(it, args, last) {
    Step step = run(env, *it);
    env = step.env;
    rc_rel(step.val); // value ignored.
  };
  return run_step(env, args.els[last]); // TCO.
}


static Step run_Scope(Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 1, "SCOPE requires 1 argument; received %i", args.len);
  Obj body = args.els[0];
  // TODO: create new env frame.
  return run_step(env, body); // TCO.
}


static Step run_Let(Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 2, "LET requires 2 arguments; received %i", args.len);
  Obj sym = args.els[0];
  Obj expr = args.els[1];
  exc_check(obj_is_sym(sym) && !sym_is_special(sym),
    "LET requires argument 1 to be a bindable sym; received: %o", sym);
  Step step = run(env, expr);
  Obj env1 = env_bind(step.env, rc_ret_val(sym), rc_ret(step.val)); // owns env, sym, val.
  return mk_step(env1, step.val);
}


static Step run_If(Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 3, "IF requires 3 arguments; received %i", args.len);
  Obj p = args.els[0];
  Obj t = args.els[1];
  Obj e = args.els[2];
  Step step = run(env, p);
  Obj branch = is_true(step.val) ? t : e;
  rc_rel(step.val);
  return run_step(step.env, branch); // TCO.
}


static Step run_Fn(Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 6, "FN requires 6 arguments; received %i", args.len);
  Obj name      = args.els[0];
  Obj is_native = args.els[1];
  Obj is_macro  = args.els[2];
  Obj pars      = args.els[3];
  Obj ret_type  = args.els[4];
  Obj body      = args.els[5];
  exc_check(obj_is_sym(name),  "FN: name is not a Sym: %o", name);
  exc_check(obj_is_bool(is_macro), "FN: is-macro is not a Bool: %o", is_macro);
  exc_check(obj_is_struct(pars), "FN: parameters is not a Struct: %o", pars);
  exc_check(struct_len(pars) > 0 && struct_el(pars, 0).u == s_Syn_seq.u,
    "FN: parameters is not a sequence literal: %o", pars);
  // TODO: check all pars.
  Obj f = struct_new_raw(rc_ret(s_Func), 7);
  Obj* els = struct_els(f);
  els[0] = rc_ret_val(name);
  els[1] = rc_ret_val(is_native);
  els[2] = rc_ret_val(is_macro);
  els[3] = rc_ret(env);
  els[4] = struct_slice_from(rc_ret(pars), 1); // remove the syntax type.
  els[5] = rc_ret(ret_type);
  els[6] = rc_ret(body);
  return mk_step(env, f);
}


static Step run_Syn_struct_boot(Obj env, Mem args) {
  // owns env.
  check(args.len > 0, "STRUCT_BOOT form is empty");
  Obj s = struct_new_raw(rc_ret(s_Obj), args.len); // TODO: correct type.
  Obj* els = struct_els(s);
  for_in(i, args.len) {
    els[i] = rc_ret(args.els[i]);
  }
  Step step = run(env, s);
  rc_rel(s);
  return step;
}


static Step run_Syn_seq(Obj env, Mem args) {
  // owns env.
  Obj s = struct_new_raw(rc_ret(s_Vec_Obj), args.len); // TODO: corrrect type.
  Obj* els = struct_els(s);
  for_in(i, args.len) {
    Step step = run(env, args.els[i]);
    env = step.env;
    els[i] = step.val;
  }
  return mk_step(env, s);
}


typedef struct {
  Obj caller_env;
  Obj callee_env;
} Call_envs;


static Call_envs run_bind_args(Obj env, Obj callee_env, Obj func, Mem pars, Mem args,
  Bool is_expand) {
  // owns env, callee_env.
  // NOTE: env is the caller env.
  Int i_args = 0;
  Bool has_variad = false;
  for_in(i_pars, pars.len) {
    Obj par = pars.els[i_pars];
    exc_check(obj_type(par).u == s_Par.u, "function %o parameter %i is malformed: %o (%o)",
      struct_el(func, 0), i_pars, par, obj_type(par));
    Obj* par_els = struct_els(par);
    Obj par_is_variad = par_els[0];
    Obj par_sym = par_els[1];
    //Obj par_type = par_els[2];
    Obj par_expr = par_els[3];
    if (!bool_is_true(par_is_variad)) { // label.
      Obj arg;
      if (i_args < args.len) {
        arg = args.els[i_args];
        i_args++;
      } else if (!is(par_expr, s_void)) {
        arg = par_expr;
      } else {
        error_obj("function received too few arguments", struct_el(func, 0));
      }
      Obj val;
      if (is_expand) {
        val = rc_ret(arg);
      } else {
        Step step = run(env, arg);
        env = step.env;
        val = step.val;
      }
      callee_env = env_bind(callee_env, rc_ret_val(par_sym), val);
    } else { // variad.
      check_obj(!has_variad, "function has multiple variad parameters", struct_el(func, 0));
      has_variad = true;
      Int variad_count = 0;
      for_imn(i, i_args, args.len) {
        Obj arg = args.els[i];
        if (obj_type(arg).u == s_Par.u) break;
        variad_count++;
      }
      Obj variad_val = struct_new_raw(rc_ret(s_Vec_Obj), variad_count);
      it_struct(it, variad_val) {
        Obj arg = args.els[i_args++];
        if (is_expand) {
          *it = rc_ret(arg);
        } else {
          Step step = run(env, arg);
          env = step.env;
          *it = step.val;
        }
      }
      callee_env = env_bind(callee_env, rc_ret_val(par_sym), variad_val);
    }
  }
  check_obj(i_args == args.len, "function received too many arguments", struct_el(func, 0));
  return (Call_envs){.caller_env=env, .callee_env=callee_env};
}


static Step run_call_native(Obj env, Obj func, Mem args, Bool is_expand) {
  // owns env, func.
  Mem m = struct_mem(func);
  assert(m.len == 7);
  Obj name      = m.els[0];
  //Obj is_native = m.els[1];
  Obj is_macro  = m.els[2];
  Obj lex_env   = m.els[3];
  Obj pars      = m.els[4];
  Obj ret_type  = m.els[5];
  Obj body      = m.els[6];
  if (is_expand) {
    exc_check(bool_is_true(is_macro), "cannot expand function: %o", name);
  } else {
    exc_check(!bool_is_true(is_macro), "cannot call macro: %o", name);
  }
  exc_check(obj_is_sym(name), "function name is not a Sym: %o", name);
  exc_check(obj_is_env(lex_env), "function %o env is not an Env: %o", name, lex_env);
  exc_check(obj_is_struct(pars), "function %o pars is not a Struct: %o", name, pars);
  exc_check(is(ret_type, s_nil), "function %o ret-type is non-nil: %o", name, ret_type);
  Obj callee_env = env_push_frame(rc_ret(lex_env), rc_ret(name));
  // TODO: change env_push_frame src from name to whole func?
  callee_env = env_bind(callee_env, rc_ret_val(s_self), rc_ret(func)); // bind self.
  Call_envs envs = run_bind_args(env, callee_env, func, struct_mem(pars), args, is_expand); // owns env, callee_env.
  // NOTE: because func is bound to self in callee_env, and func contains body,
  // we can release func now and still safely return the unretained body as .tco_code.
  rc_rel(func);
#if 1 // TCO.
  // caller will own .env and .val, but not .tco_code.
  return (Step){.env=envs.caller_env, .val=envs.callee_env, .tco_code=body}; // TCO.
#else // NO TCO.
  Step step = run(envs.callee_env, body); // owns callee_env.
  rc_rel(step.env);
  return mk_step(envs.caller_env, step.val); // NO TCO.
#endif
}


static Step run_call_host(Obj env, Obj func, Mem args) {
  // owns env, func.
  Mem m = struct_mem(func);
  assert(m.len == 7);
  Obj name      = m.els[0];
  //Obj is_native = m.els[1];
  Obj is_macro  = m.els[2];
  Obj lex_env   = m.els[3];
  Obj pars      = m.els[4];
  Obj ret_type  = m.els[5];
  Obj body      = m.els[6];
  exc_check(!bool_is_true(is_macro), "host function appears to be a macro: %o", func);
  exc_check(obj_is_sym(name), "host function name is not a Sym: %o", name);
  exc_check(obj_is_env(lex_env), "host function %o env is not an Env: %o", name, lex_env);
  exc_check(obj_is_struct(pars), "host function %o pars is not a Struct: %o", name, pars);
  exc_check(is(ret_type, s_nil), "host function %o ret-type is non-nil: %o", name, ret_type);
  exc_check(obj_is_ptr(body), "host function %o body is not a Ptr: %o", name, body);
  Int len_pars = struct_len(pars);
  exc_check(args.len == len_pars, "host function expects %i argument%s; received %i",
    len_pars, (len_pars == 1 ? "" : "s"), args.len);
  // TODO: check arg types against parameters; use run_bind_args?
  Func_host_ptr f_ptr = cast(Func_host_ptr, ptr_val(body));
  rc_rel(func);
  Obj arg_vals[args.len]; // requires variable-length-array support from compiler.
  for_in(i, args.len) {
    Step step = run(env, args.els[i]);
    arg_vals[i] = step.val;
  }
  return mk_step(env, f_ptr(env, mem_mk(args.len, arg_vals))); // TODO: add TCO for host_run?
}


static Step run_Call(Obj env, Mem args) {
  // owns env.
  check(args.len > 0, "call is empty");
  Obj callee = args.els[0];
  Step step = run(env, callee);
  Obj func = step.val;
  exc_check(obj_is_struct(func), "object is not callable: %o", func);
  Mem m = struct_mem(func);
  exc_check(m.len == 7, "function is malformed (length is not 7): %o", func);
  Obj is_native = m.els[1];
  if (bool_is_true(is_native)) {
    return run_call_native(env, func, mem_next(args), false); // TCO.
  } else {
    return run_call_host(env, func, mem_next(args)); // TODO: make TCO?
  }
}


static Step run_Struct(Obj env, Obj code) {
  // owns env.
  Mem m = struct_mem(code);
  Obj form = m.els[0];
  Obj_tag ot = obj_tag(form);
  if (ot == ot_sym) {
    Int si = sym_index(form); // Int type avoids incomplete enum switch error.
#define EVAL_FORM(s) case si_##s: return run_##s(env, mem_next(m))
    switch (si) {
      EVAL_FORM(Quo);
      EVAL_FORM(Do);
      EVAL_FORM(Scope);
      EVAL_FORM(Let);
      EVAL_FORM(If);
      EVAL_FORM(Fn);
      EVAL_FORM(Syn_struct_boot);
      EVAL_FORM(Syn_seq);
      EVAL_FORM(Call);
    }
#undef EVAL_FORM
  }
  exc_raise("cannot call Struct object: %o", code);
}


// TODO: use or remove; fix comments.
static const Chars_const trace_run_prefix    = "▿ "; // during trace, printed before each run; white_down_pointing_small_triangle.
static const Chars_const trace_val_prefix    = "◦ "; // during trace, printed before calling continuation; white_bullet.
static const Chars_const trace_apply_prefix  = "▹ "; // called before each call apply;  white_right_pointing_small_triangle.


static Step run_step(Obj env, Obj code) {
  // owns env.
#if VERBOSE_EVAL
    err(trace_run_prefix); dbg(code); // TODO: improve this?
#endif
  Obj_tag ot = obj_tag(code);
  if (ot == ot_int) {
    return mk_step(env, rc_ret_val(code)); // self-evaluating.
  }
  if (ot == ot_sym) {
    if (obj_is_data_word(code)) {
      return mk_step(env, rc_ret_val(code)); // self-evaluating.
    } else {
      return run_sym(env, code);
    }
  }
  switch (ref_tag(code)) {
    case rt_Data:
      return mk_step(env, rc_ret(code)); // self-evaluating.
    case rt_Env:
      exc_raise("cannot run object: %o", code);
    case rt_Struct:
      return run_Struct(env, code);
  }
}


static Step run_tail(Step step) {
  // owns .env and .val; borrows .tco_code.
  Obj env = step.env; // hold onto the original 'next' environment for the topmost caller.
  while (!is(step.tco_code, obj0)) {
    Obj tco_env = step.val; // val field is reused in the TCO case to hold the callee env.
    step = run_step(tco_env, step.tco_code); // owns tco_env; borrows .tco_code.
    rc_rel(step.env); // the modified tco_env is immediately abandoned since tco_code is in the tail position.
  }
  step.env = env; // replace whatever modified callee env from TCO with the topmost env.
  // done. if TCO iterations occurred,
  // then the intermediate step.val objects were really tc_env, consumed by run_step.
  // the final val is the one we want to return, so we do not have to ret/rel.
  assert(is(step.tco_code, obj0));
  return step;
}


static Step run(Obj env, Obj code) {
  // owns env.
  Step step = run_step(env, code);
  return run_tail(step);
}

