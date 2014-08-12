 // Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "28-compile.h"

// struct representing the result of a step of computation.
typedef struct {
  Obj env; // the env to be passed to the next step.
  Obj val; // the result from the step just performed.
  Obj empty0; // padding; must be obj0 for polymorph test.
} Res;

typedef struct {
  Obj env; // the caller env; this struct slot must match that of Res.
  Obj callee_env; // the callee_env, to be used for the next tail step.
  Obj code; // code for the TCO step.
} Tail;

typedef union {
  Res res;
  Tail tail;
} Step;

#define mk_res(e, v) (Step){.res={.env=(e), .val=(v), .empty0=obj0}}
#define mk_tail(caller, callee, code) (Step){.tail={caller, callee, code}}

#define is_step_res(step) is(step.res.empty0, obj0)
#define is_step_tail(step) !is_step_res(step)


static Step run_sym(Trace* trace, Obj env, Obj code) {
  // owns env.
  assert(obj_is_sym(code));
  assert(!is(code, s_ILLEGAL)); // anything that returns ILLEGAL should have raised an error.
  if (code.u <= s_END_SPECIAL_SYMS.u) {
    return mk_res(env, rc_ret_val(code)); // special syms are self-evaluating.
  }
  Obj val = env_get(env, code);
  exc_check(!is(val, obj0), "lookup error: %o", code); // lookup failed.
  return mk_res(env, rc_ret(val));
}


static Step run(Int d, Trace* trace, Obj env, Obj code);

static Step run_Eval(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem args = struct_mem(code);
  exc_check(args.len == 1, "Env requires 1 argument; received %i", args.len);
  Obj expr = args.els[0];
  Step step = run(d, trace, env, expr);
  env = step.res.env;
  Obj dyn_code = step.res.val;
  // for now, perform a non-TCO run so that we do not have to retain dyn_code.
  // TODO: TCO?
  step = run(d, trace, env, dyn_code);
  rc_rel(dyn_code);
  return step;
}


static Step run_Quo(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem args = struct_mem(code);
  exc_check(args.len == 1, "Quo requires 1 argument; received %i", args.len);
  return mk_res(env, rc_ret(args.els[0]));
}


static Step run_Do(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem exprs = struct_mem(code);
  if (!exprs.len) {
    return mk_res(env, rc_ret_val(s_void));
  }
  Int last = exprs.len - 1;
  it_mem_to(it, exprs, last) {
    Step step = run(d, trace, env, *it);
    env = step.res.env;
    rc_rel(step.res.val); // value ignored.
  };
  return mk_tail(.env=env, .callee_env=env, .code=exprs.els[last]);
}


static Step run_Scope(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem args = struct_mem(code);
  exc_check(args.len == 1, "Scope requires 1 argument; received %i", args.len);
  Obj expr = args.els[0];
  Obj sub_env = env_push_frame(rc_ret(env));
  return mk_tail(.env=env, .callee_env=sub_env, .code=expr);
}


static Obj run_env_bind(Trace* trace, Obj env, Obj key, Obj val) {
  // owns env, key, val.
  Obj env1 = env_bind(env, key, val);
  exc_check(!is(env, env1), "symbol is already bound: %o", key);
  return env1;
}


static Step run_Let(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem args = struct_mem(code);
  exc_check(args.len == 2, "Let requires 2 arguments; received %i", args.len);
  Obj sym = args.els[0];
  Obj expr = args.els[1];
  exc_check(obj_is_sym(sym), "Let requires argument 1 to be a bindable sym; received: %o", sym);
  exc_check(!sym_is_special(sym), "Let cannot bind to special sym: %o", sym);
  Step step = run(d, trace, env, expr);
  Obj env1 = run_env_bind(trace, step.res.env, rc_ret_val(sym), rc_ret(step.res.val));
  return mk_res(env1, step.res.val);
}


static Step run_If(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem args = struct_mem(code);
  exc_check(args.len == 3, "If requires 3 arguments; received %i", args.len);
  Obj pred = args.els[0];
  Obj then = args.els[1];
  Obj else_ = args.els[2];
  Step step = run(d, trace, env, pred);
  env = step.res.env;
  Obj branch = is_true(step.res.val) ? then : else_;
  rc_rel(step.res.val);
  return mk_tail(.env=env, .callee_env=env, .code=branch);
}


static Step run_Fn(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem args = struct_mem(code);
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
  exc_check(is(obj_type(pars), t_Syn_seq),
    "Fn: parameters is not a sequence literal: %o", pars);
  // TODO: check all pars.
  Obj f = struct_new_raw(rc_ret(t_Func), 7);
  Obj* els = struct_els(f);
  els[0] = rc_ret_val(name);
  els[1] = rc_ret_val(is_native);
  els[2] = rc_ret_val(is_macro);
  els[3] = rc_ret(env);
  els[4] = struct_new_M_ret(rc_ret(t_Mem_Par), struct_mem(pars)); // replace the syntax type.
  els[5] = rc_ret(ret_type);
  els[6] = rc_ret(body);
  return mk_res(env, f);
}


static Step run_Syn_struct_typed(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem args = struct_mem(code);
  check(args.len > 0, "Syn-struct-typed is empty");
  Step step = run(d, trace, env, args.els[0]); // evaluate the type.
  env = step.res.env;
  Obj s = struct_new_raw(step.res.val, args.len - 1);
  Obj* els = struct_els(s);
  for_in(i, args.len - 1) {
    step = run(d, trace, env, args.els[i + 1]);
    env = step.res.env;
    els[i] = step.res.val;
  }
  return mk_res(env, s);
}


static Step run_Syn_seq_typed(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem args = struct_mem(code);
  check(args.len > 0, "Syn-seq-typed is empty");
  Step step = run(d, trace, env, args.els[0]); // evaluate the type.
  env = step.res.env;
  // TODO: derive the appropriate sequence type from the element type.
  Obj s = struct_new_raw(step.res.val, args.len - 1);
  Obj* els = struct_els(s);
  for_in(i, args.len - 1) {
    step = run(d, trace, env, args.els[i + 1]);
    env = step.res.env;
    els[i] = step.res.val;
  }
  return mk_res(env, s);
}


typedef struct {
  Obj caller_env;
  Obj callee_env;
} Call_envs;


static Call_envs run_bind_args(Int d, Trace* trace, Obj env, Obj callee_env,
  Obj func, Mem pars, Mem args, Bool is_expand) {
  // owns env (the caller env), callee_env.
  Int i_args = 0;
  Bool has_variad = false;
  for_in(i_pars, pars.len) {
    Obj par = pars.els[i_pars];
    exc_check(obj_type(par).u == t_Par.u, "function %o parameter %i is malformed: %o (%o)",
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
        Step step = run(d, trace, env, arg);
        env = step.res.env;
        val = step.res.val;
      }
      callee_env = run_env_bind(trace, callee_env, rc_ret_val(par_sym), val);
    } else { // variad.
      check(!has_variad, "function has multiple variad parameters: %o", struct_el(func, 0));
      has_variad = true;
      Int variad_count = 0;
      for_imn(i, i_args, args.len) {
        Obj arg = args.els[i];
        if (obj_type(arg).u == t_Par.u) break;
        variad_count++;
      }
      Obj variad_val = struct_new_raw(rc_ret(t_Mem_Obj), variad_count);
      it_struct(it, variad_val) {
        Obj arg = args.els[i_args++];
        if (is_expand) {
          *it = rc_ret(arg);
        } else {
          Step step = run(d, trace, env, arg);
          env = step.res.env;
          *it = step.res.val;
        }
      }
      callee_env = run_env_bind(trace, callee_env, rc_ret_val(par_sym), variad_val);
    }
  }
  check(i_args == args.len, "function received too many arguments: %o", struct_el(func, 0));
  return (Call_envs){.caller_env=env, .callee_env=callee_env};
}


static Step run_call_native(Int d, Trace* trace, Obj env, Obj call, Obj func, Bool is_expand) {
  // owns env, func.
  Mem args = mem_next(struct_mem(call));
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
  Obj callee_env = env_push_frame(rc_ret(lex_env));
  // NOTE: because func is bound to self in callee_env, and func contains body,
  // we can give func to callee_env and still safely return the unretained body as tail.code.
  callee_env = run_env_bind(trace, callee_env, rc_ret_val(s_self), func);
  // owns env, callee_env.
  Call_envs envs =
  run_bind_args(d, trace, env, callee_env, func, struct_mem(pars), args, is_expand);
#if 1 // TCO.
  // caller will own .env, .callee_env, but not .code.
  return mk_tail(.env=envs.caller_env, .callee_env=envs.callee_env, .code=body);
#else // NO TCO.
  Step step = run(d, trace, envs.callee_env, body); // owns callee_env.
  rc_rel(step.res.env);
  return mk_res(envs.caller_env, step.res.val); // NO TCO.
#endif
}


static Step run_call_host(Int d, Trace* trace, Obj env, Obj code, Obj func, Mem args) {
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
    Step step = run(d, trace, env, args.els[i]);
    env = step.res.env;
    arg_vals[i] = step.res.val;
  }
  Obj res = f_ptr(trace, env, mem_mk(args.len, arg_vals));
  return mk_res(env, res);
}


static Step run_Call(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem args = struct_mem(code);
  check(args.len > 0, "call is empty");
  Obj callee_expr = args.els[0];
  Step step = run(d, trace, env, callee_expr);
  env = step.res.env;
  Obj func = step.res.val;
  exc_check(obj_is_struct(func), "object is not callable: %o", func);
  Mem m = struct_mem(func);
  exc_check(m.len == 7, "function is malformed (length is not 7): %o", func);
  Obj is_native = m.els[1];
  if (bool_is_true(is_native)) {
    return run_call_native(d, trace, env, code, func, false); // TCO.
  } else {
    return run_call_host(d, trace, env, code, func, mem_next(args)); // TODO: make TCO?
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



static Step run_step_disp(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Obj_tag ot = obj_tag(code);
  if (ot == ot_ptr) {
    exc_raise("cannot run Ptr object: %o", code);
  }
  if (ot == ot_int) {
    return mk_res(env, rc_ret_val(code)); // self-evaluating.
  }
  if (ot == ot_sym) {
    if (obj_is_data_word(code)) {
      return mk_res(env, rc_ret_val(code)); // self-evaluating.
    } else {
      return run_sym(trace, env, code);
    }
  }
  assert(ot == ot_ref);
  Obj type = ref_type(code);
  Int ti = type_index(type); // cast to Int c type avoids incomplete enum switch error.
#define RUN(t) case ti_##t: return run_##t(d, trace, env, code)
  switch (ti) {
    case ti_Data: return mk_res(env, rc_ret(code)); // self-evaluating.
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


static Bool trace_eval = false;

static void run_err_trace(Int d, Chars_const p, Obj o) {
  if (trace_eval) {
    for_in(i, d) err(i % 4 ? " " : "|");
    errFL("%s %o", p, o);
  }
}


static Step run_step(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  run_err_trace(d, trace_run_prefix, code);
  Step step = run_step_disp(d, trace, env, code);
  return step;
}


static Step run_tail(Int d, Trace* parent_trace, Step step) {
  // in res case, owns res.env, res.val;
  // in tail case, owns tail.env, tail.callee_env; borrows tail.code.
  // note: if tail.env is tail.callee_env, it is only retained once.
  if (is_step_res(step)) {
    run_err_trace(d, trace_val_prefix, step.res.val);
    return step;
  }
  Obj base_env = step.tail.env; // hold onto the 'next' env for the topmost caller.
  Bool update_base_env = true; // false once any tail code creates a new scope.
  Trace t = {.code=obj0, .elided_step_count=-1, .next=parent_trace};
  do { // TCO step.
    run_err_trace(d, trace_tail_prefix, step.tail.code);
    // if the tail step is a scope or call, then a new callee environment is introduced.
    // otherwise, env and callee_env are identical, and that env was only retained once.
    Bool is_local_step = is(step.tail.env, step.tail.callee_env);
    if (!is_local_step) { // step is a new scope step (scope or macro/function body).
      if (update_base_env) { // ownership transferred to base_env.
        assert(is(base_env, step.tail.env));
      } else { // env is abandoned due to tail call or tail scope.
        rc_rel(step.tail.env);
      }
    }
    t.code = step.tail.code;
    t.elided_step_count++;
    step = run_step_disp(d, &t, step.tail.callee_env, step.tail.code); // consumes callee_env.
    // once a scope is introduced we no longer update base_env.
    update_base_env = update_base_env && is_local_step;
    if (update_base_env) { // base_env/env/callee_env was just consumed and is now dead.
      base_env = step.res.env; // accessing new env is valid for either res or step morphs.
    } else if (is_local_step) { // local tail step inside of a call; callee_env was consumed.
      // no-op, since base_env does not change and rc is already correct.
    } else { // new scope step (scope or macro/function body); res.env is the new callee env.
      // if the new step is a local step then it must be released.
    }
  } while (!is_step_res(step));
  if (!is(step.res.env, base_env)) {
    rc_rel(step.res.env); 
    step.res.env = base_env; // replace whatever modified callee env from TCO with the topmost env.
  }
  run_err_trace(d, trace_val_prefix, step.res.val);
  return step;
}


static Step run(Int depth, Trace* parent_trace, Obj env, Obj code) {
  // owns env.
  Int d = depth + 1;
  Trace t = {.code=code, .elided_step_count=0, .next=parent_trace};
  Step step = run_step(d, &t, env, code);
  return run_tail(d, &t, step);
}


static Step run_code(Obj env, Obj code) {
  // owns env.
  return run(-1, NULL, env, code);
}


static Obj run_macro(Trace* trace, Obj env, Obj code) {
  // owns env.
  run_err_trace(0, trace_expand_prefix, code);
  Mem args = struct_mem(code);
  check(args.len > 0, "empty macro expand");
  Obj macro_sym = mem_el(args, 0);
  check(obj_is_sym(macro_sym), "expand argument 0 must be a Sym; found: %o", macro_sym);
  Obj macro = env_get(env, macro_sym);
  if (is(macro, obj0)) { // lookup failed.
    error("macro lookup error: %o", macro_sym);
  }
  Step step = run_call_native(0, trace, env, code, rc_ret(macro), true); // owns env, macro.
  step = run_tail(0, trace, step); // handle any TCO steps.
  rc_rel(step.res.env);
  run_err_trace(0, trace_expand_val_prefix, step.res.val);
  return step.res.val;
}


