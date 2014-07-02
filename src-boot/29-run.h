 // Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "28-compile.h"


static Step run_sym(Obj env, Obj code) {
  // owns env.
  assert(obj_is_sym(code));
  assert(code.u != s_ILLEGAL.u); // anything that returns s_ILLEGAL should have raised an error.
  if (code.u <= s_END_SPECIAL_SYMS.u) {
    return mk_step(env, rc_ret_val(code)); // special syms are self-evaluating.
  }
  Obj val = env_get(env, code);
  exc_check(val.u != obj0.u, "lookup error: %o", code); // lookup failed.
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
  exc_check(args.len == 4,
    "FN requires 4 arguments: name:Sym is-macro:Bool parameters:Vec body:Obj; received %i",
    args.len);
  Obj name      = args.els[0];
  Obj is_macro  = args.els[1];
  Obj pars      = args.els[2];
  Obj body      = args.els[3];
  exc_check(obj_is_sym(name),  "FN: name is not a Sym: %o", name);
  exc_check(obj_is_bool(is_macro), "FN: is-macro is not a Bool: %o", is_macro);
  exc_check(obj_is_vec(pars), "FN: parameters is not a Vec: %o", pars);
  exc_check(vec_len(pars) > 0 && vec_ref_el(pars, 0).u == s_Seq.u,
    "FN: parameters is not a sequence literal: %o", pars);
  // TODO: check all pars.
  Obj f = vec_new_raw(5);
  Obj* els = vec_ref_els(f);
  els[0] = rc_ret_val(name);
  els[1] = rc_ret_val(is_macro);
  els[2] = rc_ret(env);
  els[3] = vec_slice_from(rc_ret(pars), 1); // HACK: remove the inital SEQ form symbol.
  els[4] = rc_ret(body);
  return mk_step(env, f);
}


static Step run_Struct_boot(Obj env, Mem args) {
  // owns env.
  check(args.len > 0, "STRUCT_BOOT form is empty");
  Obj v = vec_new_raw(args.len);
  Obj* els = vec_els(v);
  for_in(i, args.len) {
    els[i] = rc_ret(args.els[i]);
  }
  Step step = run(env, v);
  rc_rel(v);
  return step;
}


static Step run_Seq(Obj env, Mem args) {
  // owns env.
  Obj v = vec_new_raw(args.len);
  Obj* els = vec_els(v);
  for_in(i, args.len) {
    Step step = run(env, args.els[i]);
    env = step.env;
    els[i] = step.val;
  }
  return mk_step(env, v);
}


typedef struct {
  Obj caller_env;
  Obj callee_env;
} Call_envs;


static Call_envs run_bind_args(Obj caller_env, Obj callee_env, Obj func, Mem pars, Mem args,
  Bool is_expand) {
  // owns caller_env, callee_env.
  Int i_args = 0;
  Bool has_variad = false;
  for_in(i_pars, pars.len) {
    Obj par = pars.els[i_pars];
    check_obj(obj_is_par(par), "function parameter is malformed", vec_ref_el(func, 0));
    Obj* par_els = vec_ref_els(par);
    Obj par_kind = par_els[0]; // Label or Variad.
    Obj par_sym = par_els[1];
    //Obj par_type = par_els[2];
    Obj par_expr = par_els[3];
    if (par_kind.u == s_Label.u) {
      Obj arg;
      if (i_args < args.len) {
        arg = args.els[i_args];
        i_args++;
      } else if (par_expr.u != s_nil.u) { // TODO: change the default value to be unwritable?
        arg = par_expr;
      } else {
        error_obj("function received too few arguments", vec_ref_el(func, 0));
      }
      Obj val;
      if (is_expand) {
        val = rc_ret(arg);
      } else {
        Step step = run(caller_env, arg);
        caller_env = step.env;
        val = step.val;
      }
      callee_env = env_bind(callee_env, rc_ret_val(par_sym), val);
    } else {
      assert(par_kind.u == s_Variad.u);
      check_obj(!has_variad, "function has multiple variad parameters", vec_ref_el(func, 0));
      has_variad = true;
      Int variad_count = 0;
      for_imn(i, i_args, args.len) {
        Obj arg = args.els[i];
        if (obj_is_par(arg)) break;
        variad_count++;
      }
      Obj variad_val = vec_new_raw(variad_count);
      it_vec(it, variad_val) {
        Obj arg = args.els[i_args++];
        if (is_expand) {
          *it = rc_ret(arg);
        } else {
          Step step = run(caller_env, arg);
          caller_env = step.env;
          *it = step.val;
        }
      }
      callee_env = env_bind(callee_env, rc_ret_val(par_sym), variad_val);
    }
  }
  check_obj(i_args == args.len, "function received too many arguments", vec_ref_el(func, 0));
  return (Call_envs){.caller_env=caller_env, .callee_env=callee_env};
}


static Step run_call_native(Obj env, Obj func, Mem args, Bool is_expand) {
  // owns env, func.
  // a native (interpreted, not hosted) function/macro is a vec consisting of:
  //  name:Sym
  //  is-macro:Bool
  //  pars:Vec
  //  body:Expr
  //  env:Env (currently implemented as a nested Vec structure, likely to change)
  Mem m = vec_mem(func);
  exc_check(m.len == 5, "function is malformed (length is not 5): %o", func);
  Obj name      = m.els[0];
  Obj is_macro  = m.els[1];
  Obj lex_env   = m.els[2];
  Obj pars      = m.els[3];
  Obj body      = m.els[4];
  if (is_expand) {
    exc_check(bool_is_true(is_macro), "cannot expand function: %o", name);
  } else {
    exc_check(!bool_is_true(is_macro), "cannot call macro: %o", name);
  }
  exc_check(obj_is_sym(name), "function is malformed (name is not a Sym): %o", name);
  exc_check(obj_is_env(lex_env), "function %o is malformed (env is not an Env): %o", name, lex_env);
  exc_check(obj_is_vec(pars), "function %o is malformed (parameters is not a Vec): %o", name, pars);
  Obj callee_env = env_push_frame(rc_ret(lex_env), rc_ret(name));
  // TODO: change env_push_frame src from name to whole func?
  callee_env = env_bind(callee_env, rc_ret_val(s_self), rc_ret(func)); // bind self.
  Call_envs envs = run_bind_args(env, callee_env, func, vec_mem(pars), args, is_expand); // owns env, callee_env.
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
  Int len_pars = func.func_host->len_pars;
  Func_host_ptr f_ptr = func.func_host->ptr;
  // len_pars == -1 indicates a variadic function.
  exc_check(args.len == len_pars || len_pars == -1,
    "host function expects %i argument%s; received %i",
    len_pars, (len_pars == 1 ? "" : "s"), args.len);
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
  Obj_tag ot = obj_tag(func);
  exc_check(ot == ot_ref, "object is not callable: %o", func);
  Ref_tag rt = ref_tag(func);
  switch (cast(Uns, rt)) { // appease -Wswitch-enum.
    case rt_Vec:        return run_call_native(env, func, mem_next(args), false); // TCO.
    case rt_Func_host:  return run_call_host(env, func, mem_next(args)); // TODO: make TCO?
    default: exc_raise("object is not callable: %o", func);
  }
}


static Step run_Vec(Obj env, Obj code) {
  // owns env.
  Mem m = vec_mem(code);
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
      EVAL_FORM(Struct_boot);
      EVAL_FORM(Seq);
      EVAL_FORM(Call);
    }
#undef EVAL_FORM
  }
  exc_raise("cannot call Vec object: %o", code);
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
    case rt_Vec:
      return run_Vec(env, code);
    case rt_Data:
      return mk_step(env, rc_ret(code)); // self-evaluating.
    case rt_Env:
    case rt_File:
    case rt_Func_host:
      exc_raise("cannot run object: %o", code);
  }
}


static Step run_tail(Step step) {
  // owns .env and .val; borrows .tco_code.
  Obj env = step.env; // hold onto the original 'next' environment for the topmost caller.
  while (step.tco_code.u != obj0.u) {
    Obj tco_env = step.val; // val field is reused in the TCO case to hold the callee env.
    step = run_step(tco_env, step.tco_code); // owns tco_env; borrows .tco_code.
    rc_rel(step.env); // the modified tco_env is immediately abandoned since tco_code is in the tail position.
  }
  step.env = env; // replace whatever modified callee env from TCO with the topmost env.
  // done. if TCO iterations occurred,
  // then the intermediate step.val objects were really tc_env, consumed by run_step.
  // the final val is the one we want to return, so we do not have to ret/rel.
  assert(step.tco_code.u == obj0.u);
  return step;
}


static Step run(Obj env, Obj code) {
  // owns env.
  Step step = run_step(env, code);
  return run_tail(step);
}
