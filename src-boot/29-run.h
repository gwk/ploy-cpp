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
  Mem m = struct_mem(code);
  exc_check(m.len == 1, "Env requires 1 field; received %i", m.len);
  Obj expr = m.els[0];
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
  Mem m = struct_mem(code);
  exc_check(m.len == 1, "Quo requires 1 field; received %i", m.len);
  return mk_res(env, rc_ret(m.els[0]));
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
  Mem m = struct_mem(code);
  exc_check(m.len == 1, "Scope requires 1 field; received %i", m.len);
  Obj expr = m.els[0];
  Obj sub_env = env_push_frame(rc_ret(env));
  return mk_tail(.env=env, .callee_env=sub_env, .code=expr);
}


static Obj run_env_bind(Trace* trace, Bool is_mutable, Obj env, Obj key, Obj val) {
  // owns env, key, val.
  Obj env1 = env_bind(env, is_mutable, key, val);
  exc_check(!is(env1, obj0), "symbol is already bound: %o", key);
  return env1;
}


static Step run_Bind(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem m = struct_mem(code);
  exc_check(m.len == 3, "Bind requires 3 fields; received %i", m.len);
  Obj is_mutable = m.els[0];
  Obj sym = m.els[1];
  Obj expr = m.els[2];
  exc_check(obj_is_bool(is_mutable), "Bind requires argument 1 to be a Bool; received: %o",
    is_mutable);
  exc_check(obj_is_sym(sym), "Bind requires argument 1 to be a bindable sym; received: %o",
    sym);
  exc_check(!sym_is_special(sym), "Bind cannot bind to special sym: %o", sym);
  Step step = run(d, trace, env, expr);
  Obj env1 = run_env_bind(trace, bool_is_true(is_mutable), step.res.env, rc_ret_val(sym),
    rc_ret(step.res.val));
  return mk_res(env1, step.res.val);
}


static Step run_If(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem m = struct_mem(code);
  exc_check(m.len == 3, "If requires 3 fields; received %i", m.len);
  Obj pred = m.els[0];
  Obj then = m.els[1];
  Obj else_ = m.els[2];
  Step step = run(d, trace, env, pred);
  env = step.res.env;
  Obj branch = is_true(step.res.val) ? then : else_;
  rc_rel(step.res.val);
  return mk_tail(.env=env, .callee_env=env, .code=branch);
}


static Step run_Fn(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem m = struct_mem(code);
  exc_check(m.len == 6, "Fn requires 6 fields; received %i", m.len);
  Obj name      = m.els[0];
  Obj is_native = m.els[1];
  Obj is_macro  = m.els[2];
  Obj pars_syn  = m.els[3];
  Obj ret_type  = m.els[4];
  Obj body      = m.els[5];
  exc_check(obj_is_sym(name),  "Fn: name is not a Sym: %o", name);
  exc_check(obj_is_bool(is_macro), "Fn: is-macro is not a Bool: %o", is_macro);
  exc_check(is(obj_type(pars_syn), t_Syn_seq),
    "Fn: pars is not a sequence literal: %o", pars_syn);
  Mem m_pars_syn = struct_mem(pars_syn);
  Obj pars = struct_new_raw(rc_ret(t_Mem_Par), m_pars_syn.len);
  Mem m_pars = struct_mem(pars);
  Obj variad = rc_ret(s_void);
  for_in(i, m_pars.len) {
    Obj syn = m_pars_syn.els[i];
    Obj syn_type = obj_type(syn);
    Obj par_name;
    Obj par_type;
    Obj par_dflt;
    Bool is_variad = false;
    if (is(syn_type, t_Label)) {
      par_name = rc_ret(struct_el(syn, 0));
      par_type = rc_ret(struct_el(syn, 1));
      par_dflt = rc_ret(struct_el(syn, 2));
    } else if (is(syn_type, t_Variad)) {
      is_variad = true;
      par_name = rc_ret(struct_el(syn, 0));
      par_type = rc_ret(struct_el(syn, 1));
      par_dflt = rc_ret_val(s_void); // TODO: ? struct_new1(rc_ret(t_Syn_seq_typed), rc_ret(par_type));
    } else {
      exc_raise("Fn: parameter %i is malformed: %o", i, syn);
    }
    Obj par =  struct_new3(rc_ret(t_Par), par_name, par_type, par_dflt);
    m_pars.els[i] = par;
    if (is_variad) {
      exc_check(is(variad, s_void), "Fn: multiple variadic parameters: %o", syn);
      rc_rel(variad);
      variad = rc_ret(par);
    }
  }
  Obj f = struct_new_raw(rc_ret(t_Func), 8);
  Obj* els = struct_els(f);
  els[0] = rc_ret_val(name);
  els[1] = rc_ret_val(is_native);
  els[2] = rc_ret_val(is_macro);
  els[3] = rc_ret(env);
  els[4] = variad;
  els[5] = pars;
  els[6] = rc_ret(ret_type);
  els[7] = rc_ret(body);
  return mk_res(env, f);
}


static Step run_Syn_struct_typed(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem m = struct_mem(code);
  check(m.len > 0, "Syn-struct-typed is empty");
  Step step = run(d, trace, env, m.els[0]); // evaluate the type.
  env = step.res.env;
  Obj s = struct_new_raw(step.res.val, m.len - 1);
  Obj* els = struct_els(s);
  for_in(i, m.len - 1) {
    step = run(d, trace, env, m.els[i + 1]);
    env = step.res.env;
    els[i] = step.res.val;
  }
  return mk_res(env, s);
}


static Step run_Syn_seq_typed(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem m = struct_mem(code);
  check(m.len > 0, "Syn-seq-typed is empty");
  Step step = run(d, trace, env, m.els[0]); // evaluate the type.
  env = step.res.env;
  // TODO: derive the appropriate sequence type from the element type.
  Obj s = struct_new_raw(step.res.val, m.len - 1);
  Obj* els = struct_els(s);
  for_in(i, m.len - 1) {
    step = run(d, trace, env, m.els[i + 1]);
    env = step.res.env;
    els[i] = step.res.val;
  }
  return mk_res(env, s);
}


typedef struct {
  Obj caller_env;
  Obj callee_env;
} Call_envs;


#define UNPACK_LABEL(l) \
Obj* l##_els = struct_els(l); \
Obj l##_el_name = l##_els[0]; \
Obj l##_el_type = l##_els[1]; \
Obj l##_el_dflt = l##_els[2]

#define UNPACK_VARIAD(v) \
Obj* v##_els = struct_els(v); \
Obj v##_el_expr = v##_els[0]; \
Obj v##_el_type = v##_els[1]

#define UNPACK_PAR(p) \
Obj* p##_els = struct_els(p); \
Obj p##_el_name = p##_els[0]; \
Obj p##_el_type = p##_els[1]; \
Obj p##_el_dflt = p##_els[2]


static Call_envs run_bind_arg(Int d, Trace* trace, Obj env, Obj callee_env, Bool is_expand,
  Obj par_name, Obj arg_expr) {
  // owns env (the caller env), callee_env.
  // bind an argument expression to a name.
  Obj val;
  if (is_expand) {
    val = rc_ret(arg_expr);
  } else {
    Step step = run(d, trace, env, arg_expr);
    env = step.res.env;
    val = step.res.val;
  }
  callee_env = run_env_bind(trace, false, callee_env, rc_ret_val(par_name), val);
  return (Call_envs){.caller_env=env, .callee_env=callee_env};
}


static Call_envs run_bind_par(Int d, Trace* trace, Obj env, Obj callee_env, Bool is_expand,
  Obj par, Mem args, Int* i_args, Obj func) {
  UNPACK_PAR(par); UNUSED_VAR(par_el_type);
  Obj arg_expr;
  if (*i_args < args.len) {
    Obj arg = args.els[*i_args];
    *i_args += 1;
    Obj arg_type = obj_type(arg);
    if (is(arg_type, t_Label)) {
      UNPACK_LABEL(arg); UNUSED_VAR(arg_el_type);
      exc_check(is(par_el_name, arg_el_name), "parameter %o does not match argument: %o",
        par_el_name, arg_el_name);
      // TODO: check type.
      arg_expr = arg_el_dflt;
    } else if (is(arg_type, t_Variad)) {
      exc_raise("splat not implemented: %o", arg);
    } else { // simple arg expr.
      arg_expr = arg;
    }
  } else if (!is(par_el_dflt, s_void)) { // use parameter default expression.
    arg_expr = par_el_dflt;
  } else {
    error("function %o received too few arguments", struct_el(func, 0));
  }
  return run_bind_arg(d, trace, env, callee_env, is_expand, par_el_name, arg_expr);
}


static Call_envs run_bind_variad(Int d, Trace* trace, Obj env, Obj callee_env, Bool is_expand,
  Obj par, Mem args, Int* i_args) {
  // owns env (the caller env), callee_env.
  // bind args to a variad parameter.
  // note: it would be nice to implement this without allocating a temporary array;
  // however i can't see how to do so while still maintaining evaluation order of arguments,
  // because we cannot count the values in a splat argument until it has been evaluated.
  UNPACK_PAR(par); UNUSED_VAR(par_el_type); UNUSED_VAR(par_el_dflt);
  Array vals = array0;
  while (*i_args < args.len) {
    Obj arg = args.els[*i_args];
    if (is(obj_type(arg), t_Label)) {
      break; // terminate variad.
    } else if (is(obj_type(arg), t_Variad)) { // splat variad argument into variad parameter.
      UNPACK_VARIAD(arg);
      exc_check(is(arg_el_type, s_void), "splat argument %i has extraneous type: %o",
        *i_args, arg);
      exc_raise("variadic splat not implemented: %o", arg_el_expr);
    } else { // arg is simple (not label or variad).
      if (is_expand) {
        array_append(&vals, rc_ret(arg));
      } else {
        Step step = run(d, trace, env, arg);
        env = step.res.env;
        array_append(&vals, step.res.val);
      }
    }
    *i_args += 1;
  }
  Obj variad_val = struct_new_raw(rc_ret(t_Mem_Obj), vals.mem.len); // TODO: set correct type.
  Obj* els = struct_els(variad_val);
  for_in(i, vals.mem.len) {
    els[i] = mem_el_move(vals.mem, i);
  }
  mem_dealloc(vals.mem);
  callee_env = run_env_bind(trace, false, callee_env, rc_ret_val(par_el_name), variad_val);
  return (Call_envs){.caller_env=env, .callee_env=callee_env};
}


static Call_envs run_bind_args(Int d, Trace* trace, Obj env, Obj callee_env,
  Obj func, Obj variad, Mem pars, Mem args, Bool is_expand) {
  // owns env (the caller env), callee_env.
  Int i_args = 0;
  Bool has_variad = false;
  for_in(i_pars, pars.len) {
    Obj par = pars.els[i_pars];
    exc_check(is(obj_type(par), t_Par), "function %o parameter %i is malformed: %o",
      struct_el(func, 0), i_pars, par);
    if (is(par, variad)) {
      exc_check(!has_variad, "function has multiple variad parameters: %o", struct_el(func, 0));
      has_variad = true;
      Call_envs envs =
      run_bind_variad(d, trace, env, callee_env, is_expand, par, args, &i_args);
      env = envs.caller_env;
      callee_env = envs.callee_env;
    } else {
      Call_envs envs =
      run_bind_par(d, trace, env, callee_env, is_expand, par, args, &i_args, func);
      env = envs.caller_env;
      callee_env = envs.callee_env;
    }
  }
  check(i_args == args.len, "function %o received too many arguments", struct_el(func, 0));
  return (Call_envs){.caller_env=env, .callee_env=callee_env};
}


static Step run_call_func(Int d, Trace* trace, Obj env, Obj call, Obj func, Bool is_expand) {
  // owns env, func.
  Mem args = mem_next(struct_mem(call));
  Mem m = struct_mem(func);
  assert(m.len == 8);
  Obj name      = m.els[0];
  Obj is_native = m.els[1];
  Obj is_macro  = m.els[2];
  Obj lex_env   = m.els[3];
  Obj variad    = m.els[4];
  Obj pars      = m.els[5];
  Obj ret_type  = m.els[6];
  Obj body      = m.els[7];
  exc_check(obj_is_sym(name), "function name is not a Sym: %o", name);
  exc_check(obj_is_bool(is_native), "function is-native is not a Bool: %o", is_native);
  exc_check(obj_is_bool(is_macro), "function is-macro is not a Bool: %o", is_macro);
  // TODO: check that variad is an Expr.
  exc_check(obj_is_env(lex_env), "function %o env is not an Env: %o", name, lex_env);
  exc_check(is(obj_type(pars), t_Mem_Par), "function %o pars is not Mem-Par: %o", name, pars);
  // TODO: check that ret-type is a Type.
  exc_check(is(ret_type, s_nil), "function %o ret-type is non-nil: %o", name, ret_type);
  if (is_expand) {
    exc_check(bool_is_true(is_macro), "cannot expand function: %o", name);
  } else {
    exc_check(!bool_is_true(is_macro), "cannot call macro: %o", name);
  }
  Obj callee_env = env_push_frame(rc_ret(lex_env));
  // NOTE: because func is bound to self in callee_env, and func contains body,
  // we can give func to callee_env and still safely return the unretained body as tail.code.
  callee_env = run_env_bind(trace, false, callee_env, rc_ret_val(s_self), func);
  // owns env, callee_env.
  Call_envs envs =
  run_bind_args(d, trace, env, callee_env, func, variad, struct_mem(pars), args, is_expand);
  env = envs.caller_env;
  callee_env = envs.callee_env;
  if (bool_is_true(is_native)) {
#if 1 // TCO.
  // caller will own .env, .callee_env, but not .code.
  return mk_tail(.env=env, .callee_env=callee_env, .code=body);
#else // NO TCO.
  Step step = run(d, trace, callee_env, body); // owns callee_env.
  rc_rel(step.res.env);
  return mk_res(env, step.res.val); // NO TCO.
#endif
  } else { // host function.
    exc_check(obj_is_ptr(body), "host function %o body is not a Ptr: %o", name, body);
    Func_host_ptr f_ptr = cast(Func_host_ptr, ptr_val(body));
    Obj res = f_ptr(trace, callee_env);
    rc_rel(callee_env);
    return mk_res(envs.caller_env, res);
  }
}


static Step run_Call(Int d, Trace* trace, Obj env, Obj code) {
  // owns env.
  Mem m = struct_mem(code);
  check(m.len > 0, "call is empty");
  Obj callee_expr = m.els[0];
  Step step = run(d, trace, env, callee_expr);
  env = step.res.env;
  Obj func = step.res.val;
  Obj type = obj_type(func);
  if (is(type, t_Func)) {
    return run_call_func(d, trace, env, code, func, false); // TCO.
  }
  exc_raise("object is not callable: %o", func);
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
    case ti_Data:
    case ti_Accessor:
    case ti_Mutator:
      return mk_res(env, rc_ret(code)); // self-evaluating.
    RUN(Eval);
    RUN(Quo);
    RUN(Do);
    RUN(Scope);
    RUN(Bind);
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
  Mem m = struct_mem(code);
  check(m.len > 0, "empty macro expand");
  Obj macro_sym = mem_el(m, 0);
  check(obj_is_sym(macro_sym), "expand argument 0 must be a Sym; found: %o", macro_sym);
  Obj macro = env_get(env, macro_sym);
  if (is(macro, obj0)) { // lookup failed.
    error("macro lookup error: %o", macro_sym);
  }
  Step step = run_call_func(0, trace, env, code, rc_ret(macro), true); // owns env, macro.
  step = run_tail(0, trace, step); // handle any TCO steps.
  rc_rel(step.res.env);
  run_err_trace(0, trace_expand_val_prefix, step.res.val);
  return step.res.val;
}


