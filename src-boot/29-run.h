 // Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "28-compile.h"


typedef struct _Trace {
  Obj src_loc;
  struct _Trace* next;
} Trace;


// struct representing a step of the interpreted computation.
// in the normal case, it contains the environment and value resulting from the previous step.
// in the tail-call optimization (TCO) case, it contains the tail step to perform.
typedef struct {
  Obj env; // the env to be passed to the next step, after any TCO steps; the new caller env.
  Obj val; // the result from the previous step, or the env to be passed to the TCO step.
  Obj tco_code; // code for the TCO step.
} Step;

#define mk_step(e, v) (Step){.env=(e), .val=(v), .tco_code=obj0}


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


static Step run(Int d, Trace* t, Obj env, Obj code);

static Step run_Eval(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 1, "Env requires 1 argument; received %i", args.len);
  Obj expr = args.els[0];
  Step step = run(d, t, env, expr);
  env = step.env;
  Obj code = step.val;
  step = run(d, t, env, code);
  rc_rel(code);
  return step;
}


static Step run_Quo(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 1, "Quo requires 1 argument; received %i", args.len);
  return mk_step(env, rc_ret(args.els[0]));
}


static Step run_step(Int d, Trace* t, Obj env, Obj code);

static Step run_Do(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  if (!args.len) {
    return mk_step(env, rc_ret_val(s_void));
  }
  Int last = args.len - 1;
  it_mem_to(it, args, last) {
    Step step = run(d, t, env, *it);
    env = step.env;
    rc_rel(step.val); // value ignored.
  };
  return run_step(d, t, env, args.els[last]); // TCO.
}


static Step run_Scope(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 1, "Scope requires 1 argument; received %i", args.len);
  Obj body = args.els[0];
  Obj sub_env = env_push_frame(rc_ret(env), data_new_from_chars(cast(Chars, "<scope>")));
  // caller will own .env and .val, but not .tco_code.
  return (Step){.env=env, .val=sub_env, .tco_code=body}; // TCO.
}


static Step run_Let(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 2, "Let requires 2 arguments; received %i", args.len);
  Obj sym = args.els[0];
  Obj expr = args.els[1];
  exc_check(obj_is_sym(sym), "Let requires argument 1 to be a bindable sym; received: %o", sym);
  exc_check(!sym_is_special(sym), "Let cannot bind to special sym: %o", sym);
  Step step = run(d, t, env, expr);
  Obj env1 = env_bind(step.env, rc_ret_val(sym), rc_ret(step.val)); // owns env, sym, val.
  return mk_step(env1, step.val);
}


static Step run_If(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 3, "If requires 3 arguments; received %i", args.len);
  Obj pred = args.els[0];
  Obj then = args.els[1];
  Obj else_ = args.els[2];
  Step step = run(d, t, env, pred);
  Obj branch = is_true(step.val) ? then : else_;
  rc_rel(step.val);
  return run_step(d, t, step.env, branch); // TCO.
}


static Step run_Fn(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  exc_check(args.len == 6, "Fn requires 6 arguments; received %i", args.len);
  Obj name      = args.els[0];
  Obj is_native = args.els[1];
  Obj is_macro  = args.els[2];
  Obj pars      = args.els[3];
  Obj ret_type  = args.els[4];
  Obj body      = args.els[5];
  exc_check(obj_is_sym(name),  "Fn: name is not a Sym: %o", name);
  exc_check(obj_is_bool(is_macro), "Fn: is-macro is not a Bool: %o", is_macro);
  exc_check(obj_is_struct(pars), "Fn: parameters is not a Struct: %o", pars);
  exc_check(is(obj_type(pars), s_Syn_seq),
    "Fn: parameters is not a sequence literal: %o", pars);
  // TODO: check all pars.
  Obj f = struct_new_raw(rc_ret(s_Func), 7);
  Obj* els = struct_els(f);
  els[0] = rc_ret_val(name);
  els[1] = rc_ret_val(is_native);
  els[2] = rc_ret_val(is_macro);
  els[3] = rc_ret(env);
  els[4] = struct_new_M_ret(rc_ret(s_Mem_Par), struct_mem(pars)); // replace the syntax type.
  els[5] = rc_ret(ret_type);
  els[6] = rc_ret(body);
  return mk_step(env, f);
}


static Step run_Syn_struct_typed(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  check(args.len > 0, "Syn-struct is empty");
  Step step = run(d, t, env, args.els[0]); // evaluate the type.
  env = step.env;
  Obj s = struct_new_raw(step.val, args.len - 1);
  Obj* els = struct_els(s);
  for_in(i, args.len - 1) {
    step = run(d, t, env, args.els[i + 1]);
    env = step.env;
    els[i] = step.val;
  }
  return mk_step(env, s);
}


static Step run_Syn_seq_typed(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  check(args.len > 0, "Syn-struct is empty");
  Step step = run(d, t, env, args.els[0]); // evaluate the type.
  env = step.env;
  // TODO: derive the appropriate sequence type from the element type.
  Obj s = struct_new_raw(step.val, args.len - 1);
  Obj* els = struct_els(s);
  for_in(i, args.len - 1) {
    step = run(d, t, env, args.els[i + 1]);
    env = step.env;
    els[i] = step.val;
  }
  return mk_step(env, s);
}


typedef struct {
  Obj caller_env;
  Obj callee_env;
} Call_envs;


static Call_envs run_bind_args(Int d, Trace* t, Obj env, Obj callee_env,
  Obj func, Mem pars, Mem args, Bool is_expand) {
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
        error("function received too few arguments: %o", struct_el(func, 0));
      }
      Obj val;
      if (is_expand) {
        val = rc_ret(arg);
      } else {
        Step step = run(d, t, env, arg);
        env = step.env;
        val = step.val;
      }
      callee_env = env_bind(callee_env, rc_ret_val(par_sym), val);
    } else { // variad.
      check(!has_variad, "function has multiple variad parameters: %o", struct_el(func, 0));
      has_variad = true;
      Int variad_count = 0;
      for_imn(i, i_args, args.len) {
        Obj arg = args.els[i];
        if (obj_type(arg).u == s_Par.u) break;
        variad_count++;
      }
      Obj variad_val = struct_new_raw(rc_ret(s_Mem_Obj), variad_count);
      it_struct(it, variad_val) {
        Obj arg = args.els[i_args++];
        if (is_expand) {
          *it = rc_ret(arg);
        } else {
          Step step = run(d, t, env, arg);
          env = step.env;
          *it = step.val;
        }
      }
      callee_env = env_bind(callee_env, rc_ret_val(par_sym), variad_val);
    }
  }
  check(i_args == args.len, "function received too many arguments: %o", struct_el(func, 0));
  return (Call_envs){.caller_env=env, .callee_env=callee_env};
}


static Step run_call_native(Int d, Trace* t, Obj env, Obj func, Mem args, Bool is_expand) {
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
  // owns env, callee_env.
  Call_envs envs =
  run_bind_args(d, t, env, callee_env, func, struct_mem(pars), args, is_expand);
  // NOTE: because func is bound to self in callee_env, and func contains body,
  // we can release func now and still safely return the unretained body as .tco_code.
  rc_rel(func);
#if 1 // TCO.
  // caller will own .env and .val, but not .tco_code.
  return (Step){.env=envs.caller_env, .val=envs.callee_env, .tco_code=body}; // TCO.
#else // NO TCO.
  Step step = run(d, t, envs.callee_env, body); // owns callee_env.
  rc_rel(step.env);
  return mk_step(envs.caller_env, step.val); // NO TCO.
#endif
}


static Step run_call_host(Int d, Trace* t, Obj env, Obj func, Mem args) {
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
  exc_check(args.len == len_pars, "host function expects %i argument%s; received %i; %o",
    len_pars, (len_pars == 1 ? "" : "s"), args.len, name);
  // TODO: check arg types against parameters; use run_bind_args?
  Func_host_ptr f_ptr = cast(Func_host_ptr, ptr_val(body));
  rc_rel(func);
  assert(args.len <= 3);
  Obj arg_vals[3];
  for_in(i, args.len) {
    Step step = run(d, t, env, args.els[i]);
    arg_vals[i] = step.val;
  }
  return mk_step(env, f_ptr(env, mem_mk(args.len, arg_vals)));
}


static Step run_Call(Int d, Trace* t, Obj env, Mem args) {
  // owns env.
  check(args.len > 0, "call is empty");
  Obj callee = args.els[0];
  Step step = run(d, t, env, callee);
  Obj func = step.val;
  exc_check(obj_is_struct(func), "object is not callable: %o", func);
  Mem m = struct_mem(func);
  exc_check(m.len == 7, "function is malformed (length is not 7): %o", func);
  Obj is_native = m.els[1];
  if (bool_is_true(is_native)) {
    return run_call_native(d, t, env, func, mem_next(args), false); // TCO.
  } else {
    return run_call_host(d, t, env, func, mem_next(args)); // TODO: make TCO?
  }
}


// printed before each run step; white_down_pointing_small_triangle.
static const Chars_const trace_run_prefix = "▿ ";

// printed before each run tail step; white_right_pointing_small_triangle.
static const Chars_const trace_tail_prefix  = "▹ ";

// printed after each run step; white_bullet.
static const Chars_const trace_val_prefix = "◦ ";

// printed before each macro expand step; white_diamond.
static const Chars_const trace_expand_prefix = "◇ ";

// printed after each macro expand step; white_small_square.
static const Chars_const trace_expand_val_prefix = "▫ ";



static Step run_step_disp(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  Obj_tag ot = obj_tag(code);
  if (ot == ot_ptr) {
    exc_raise("cannot run object: %o", code);
  }
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
  assert(ot == ot_ref);
  Obj type = ref_type(code);
  exc_check(obj_tag(type) == ot_sym, "cannot run object with non-sym type: %o", code);
  Int si = sym_index(type); // Int type avoids incomplete enum switch error.
#define RUN(s) case si_##s: return run_##s(d, t, env, struct_mem(code))
  switch (si) {
    case si_Data: return mk_step(env, rc_ret(code)); // self-evaluating.
    RUN(Eval);
    RUN(Quo);
    RUN(Do);
    RUN(Scope);
    RUN(Let);
    RUN(If);
    RUN(Fn);
    RUN(Syn_struct_typed);
    RUN(Syn_seq_typed);
    RUN(Call);
  }
#undef RUN
  exc_raise("cannot run object: %o", code);
}


static Step run_step(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
#if VERBOSE_EVAL
    for_in(i, d) err(i % 4 ? " " : "|");
    errFL("%s %o", trace_run_prefix, code);
#endif
    Step step = run_step_disp(d, t, env, code);
    return step;
}


static Step run_tail(Int d, Trace* t, Step step) {
  // owns .env and .val; borrows .tco_code.
  Obj env = step.env; // hold onto the original 'next' environment for the topmost caller.
  while (!is(step.tco_code, obj0)) {
    Obj tco_env = step.val; // val field is reused in the TCO case to hold the callee env.
#if VERBOSE_EVAL
    for_in(i, d) err(i % 4 ? " " : "|");
    errFL("%s %o", trace_tail_prefix, step.tco_code);
#endif
    step = run_step_disp(d, t, tco_env, step.tco_code); // owns tco_env; borrows .tco_code.
    // the modified tco_env is immediately abandoned since tco_code is in the tail position.
    rc_rel(step.env);
  }
  step.env = env; // replace whatever modified callee env from TCO with the topmost env.
  // done. if TCO iterations occurred,
  // then the intermediate step.val objects were really tc_env, consumed by run_step.
  // the final val is the one we want to return, so we do not have to ret/rel.
  assert(is(step.tco_code, obj0));
#if VERBOSE_EVAL
    for_in(i, d) err(i % 4 ? " " : "|");
    errFL("%s %o", trace_val_prefix, step.val);
#endif
  return step;
}


static Step run(Int depth, Trace* t, Obj env, Obj code) {
  Int d = depth + 1;
  // owns env.
  Step step = run_step(d, t, env, code);
  return run_tail(d, t, step);
}


static Obj run_macro(Obj env, Obj code) {
  Trace* t = NULL;
#if VERBOSE_EVAL
  errFL("%s %o", trace_expand_prefix, code);
#endif
  Mem args = struct_mem(code);
  check(args.len > 0, "empty macro expand");
  Obj macro_sym = mem_el(args, 0);
  check(obj_is_sym(macro_sym), "expand argument 0 must be a Sym; found: %o", macro_sym);
  Obj macro = env_get(env, macro_sym);
  if (is(macro, obj0)) { // lookup failed.
    error("macro lookup error: %o", macro_sym);
  }
  Step step = run_call_native(0, t, rc_ret(env), rc_ret(macro), mem_next(args), true);
  step = run_tail(0, t, step); // handle any TCO steps.
  rc_rel(step.env);
#if VERBOSE_EVAL
    errFL("%s %o", trace_expand_val_prefix, step.val);
#endif
  return step.val;
}


