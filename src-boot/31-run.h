 // Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "30-compile.h"

// struct representing the result of a step of computation.
struct Res {
  Obj env; // the env to be passed to the next step.
  Obj val; // the result from the step just performed.
  Obj empty; // padding; must be obj0 for polymorph test.
  Res(Obj e, Obj v): env(e), val(v), empty(obj0) {}
};

struct Tail {
  Obj env; // the caller env; this struct slot must match that of Res.
  Obj callee_env; // the callee_env, to be used for the next tail step.
  Obj code; // code for the TCO step.
  Tail(Obj e, Obj ce, Obj c): env(e), callee_env(ce), code(c) {}
};

union Step {
  Res res;
  Tail tail;
  Step(Obj e, Obj v): res(e, v) {}
  Step(Obj e, Obj ce, Obj c): tail(e, ce, c) {}
};


#define is_step_res(step) !step.res.empty.vld()


static Step run_sym(Trace* t, Obj env, Obj code) {
  // owns env.
  assert(code.is_sym());
  if (code.u <= s_END_SPECIAL_SYMS.u) {
    return Step(env, rc_ret_val(code)); // special syms are self-evaluating.
  }
  Obj val = env_get(env, code);
  exc_check(val.vld(), "lookup error: %o", code); // lookup failed.
  return Step(env, rc_ret(val));
}


static Step run_Quo(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(cmpd_len(code) == 1, "Quo requires 1 field; received %i", cmpd_len(code));
  return Step(env, rc_ret(cmpd_el(code, 0)));
}


static Step run(Int depth, Trace* parent_trace, Obj env, Obj code);

static Step run_Do(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  assert(cmpd_len(code) == 1);
  Obj exprs = cmpd_el(code, 0);
  Int len = cmpd_len(exprs);
  if (!len) {
    return Step(env, rc_ret_val(s_void));
  }
  Int last = len - 1;
  for_in(i, last) {
    Step step = run(d, t, env, cmpd_el(exprs, i));
    env = step.res.env;
    rc_rel(step.res.val); // value ignored.
  };
  return Step(env, env, cmpd_el(exprs, last));
}


static Step run_Scope(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(cmpd_len(code) == 1, "Scope requires 1 field; received %i", cmpd_len(code));
  Obj expr = cmpd_el(code, 0);
  Obj sub_env = env_push_frame(rc_ret(env));
  return Step(env, sub_env, expr);
}


static Obj bind_val(Trace* t, Bool is_mutable, Obj env, Obj key, Obj val) {
  // owns env, key, val.
  Obj env1 = env_bind(env, is_mutable, false, key, val);
  exc_check(env1.vld(), "symbol is already bound: %o", key);
  return env1;
}


static Step run_Bind(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(cmpd_len(code) == 4, "Bind requires 4 fields; received %i", cmpd_len(code));
  Obj is_mutable = cmpd_el(code, 0);
  Obj is_public = cmpd_el(code, 1);
  Obj sym = cmpd_el(code, 2);
  Obj expr = cmpd_el(code, 3);
  exc_check(is_mutable.is_bool(), "Bind requires argument 1 to be a Bool; received: %o",
    is_mutable);
  exc_check(is_public.is_bool(), "Bind requires argument 2 to be a Bool; received: %o",
    is_public);
  exc_check(sym.is_sym(), "Bind requires argument 3 to be a bindable sym; received: %o",
    sym);
  exc_check(!sym_is_special(sym), "Bind cannot bind to special sym: %o", sym);
  Step step = run(d, t, env, expr);
  Obj env1 = bind_val(t, bool_is_true(is_mutable), step.res.env, rc_ret_val(sym),
    rc_ret(step.res.val));
  return Step(env1, step.res.val);
}


static Step run_If(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(cmpd_len(code) == 3, "If requires 3 fields; received %i", cmpd_len(code));
  Obj pred = cmpd_el(code, 0);
  Obj then = cmpd_el(code, 1);
  Obj else_ = cmpd_el(code, 2);
  Step step = run(d, t, env, pred);
  env = step.res.env;
  Obj branch = is_true(step.res.val) ? then : else_;
  rc_rel(step.res.val);
  return Step(env, env, branch);
}


static Step run_Fn(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(cmpd_len(code) == 5, "Fn requires 5 fields; received %i", cmpd_len(code));
  Obj is_native = cmpd_el(code, 0);
  Obj is_macro  = cmpd_el(code, 1);
  Obj pars_seq  = cmpd_el(code, 2);
  Obj ret_type  = cmpd_el(code, 3);
  Obj body      = cmpd_el(code, 4);
  exc_check(is_macro.is_bool(), "Fn: is-macro is not a Bool: %o", is_macro);
  exc_check(obj_type(pars_seq) == t_Syn_seq,
    "Fn: pars is not a sequence literal: %o", pars_seq);
  assert(cmpd_len(pars_seq) == 1);
  Obj pars_exprs = cmpd_el(pars_seq, 0);
  Obj pars = cmpd_new_raw(rc_ret(t_Arr_Par), cmpd_len(pars_exprs));
  Obj variad = rc_ret(s_void);
  for_in(i, cmpd_len(pars_exprs)) {
    Obj syn = cmpd_el(pars_exprs, i);
    Obj syn_type = obj_type(syn);
    Obj par_name;
    Obj par_type;
    Obj par_dflt;
    Bool is_variad = false;
    if (syn_type == t_Label) {
      par_name = rc_ret(cmpd_el(syn, 0));
      par_type = rc_ret(cmpd_el(syn, 1));
      par_dflt = rc_ret(cmpd_el(syn, 2));
    } else if (syn_type == t_Variad) {
      is_variad = true;
      par_name = rc_ret(cmpd_el(syn, 0));
      par_type = rc_ret(cmpd_el(syn, 1));
      par_dflt = rc_ret_val(s_void); // TODO?
    } else {
      exc_raise("Fn: parameter %i is malformed: %o", i, syn);
    }
    Obj par =  cmpd_new3(rc_ret(t_Par), par_name, par_type, par_dflt);
    cmpd_put(pars, i, par);
    if (is_variad) {
      exc_check(variad == s_void, "Fn: multiple variadic parameters: %o", syn);
      rc_rel(variad);
      variad = rc_ret(par);
    }
  }
  Obj f = cmpd_new7(rc_ret(t_Func),
    rc_ret_val(is_native),
    rc_ret_val(is_macro),
    rc_ret(env),
    variad,
    pars,
    rc_ret(ret_type),
    rc_ret(body));
  return Step(env, f);
}


#define UNPACK_LABEL(l) \
Obj l##_el_name = cmpd_el(l, 0); \
Obj l##_el_type = cmpd_el(l, 1); \
Obj l##_el_expr = cmpd_el(l, 2)

#define UNPACK_VARIAD(v) \
Obj v##_el_expr = cmpd_el(v, 0); \
Obj v##_el_type = cmpd_el(v, 1)

#define UNPACK_PAR(p) \
Obj p##_el_name = cmpd_el(p, 0); \
Obj p##_el_type = cmpd_el(p, 1); \
Obj p##_el_dflt = cmpd_el(p, 2)


static Obj bind_par(Int d, Trace* t, Obj env, Obj call, Obj par, Mem vals, Int* i_vals) {
  // owns env.
  UNPACK_PAR(par); UNUSED_VAR(par_el_type);
  Obj val;
  Int i = *i_vals;
  if (i < vals.len) {
    Obj arg_name = mem_el_move(vals, i++);
    Obj arg = mem_el_move(vals, i++);
    *i_vals = i;
    exc_check(!arg_name.vld() || par_el_name == arg_name,
      "call: %o\nparameter: %o\ndoes not match argument label %i: %o\narg: %o",
       call, par_el_name, i, arg_name, arg);
    // TODO: check type.
    val = arg;
  } else if (par_el_dflt != s_void) { // use parameter default expression.
    Step step = run(d, t, env, par_el_dflt);
    env = step.res.env;
    val = step.res.val;
  } else {
    exc_raise("call: %o\nreceived too few arguments", call);
  }
  env = bind_val(t, false, env, rc_ret_val(par_el_name), val);
  return env;
}


static Obj bind_variad(Int d, Trace* t, Obj env, Obj call, Obj par, Mem vals, Int* i_vals) {
  // owns env.
  UNPACK_PAR(par); UNUSED_VAR(par_el_type);
  exc_check(par_el_dflt == s_void, "variad parameter has non-void default argument: %o", par);
  Int start = *i_vals;
  Int end = vals.len;
  for_imns(i, start, end, 2) {
    if (mem_el(vals, i).vld()) { // labeled arg; terminate variad.
      end = i;
      break;
    }
  }
  *i_vals = end;
  Int len = (end - start) / 2;
  Obj vrd = cmpd_new_raw(rc_ret(t_Arr_Obj), len); // TODO: set correct type.
  Int j = 0;
  for_imns(i, start + 1, end, 2) {
    Obj arg = mem_el_move(vals, i);
    cmpd_put(vrd, j++, arg);
  }
  env = bind_val(t, false, env, rc_ret_val(par_el_name), vrd);
  return env;
}


static Obj run_bind_vals(Int d, Trace* t, Obj env, Obj call, Obj variad, Obj pars,
  Mem vals) {
  // owns env.
  env = bind_val(t, false, env, rc_ret_val(s_self), mem_el_move(vals, 0));
  Int i_vals = 1; // first name of the interleaved name/value pairs.
  Bool has_variad = false;
  for_in(i_pars, cmpd_len(pars)) {
    assert(i_vals <= vals.len && i_vals % 2 == 1);
    Obj par = cmpd_el(pars, i_pars);
    exc_check(obj_type(par) == t_Par, "call: %o\nparameter %i is malformed: %o",
      call, i_pars, par);
    if (par == variad) {
      exc_check(!has_variad, "call: %o\nmultiple variad parameters", call);
      has_variad = true;
      env = bind_variad(d, t, env, call, par, vals, &i_vals);
    } else {
      env = bind_par(d, t, env, call, par, vals, &i_vals);
    }
  }
  exc_check(i_vals == vals.len, "call: %o\nreceived too many arguments", call);
  return env;
}


static Step run_call_func(Int d, Trace* t, Obj env, Obj call, Mem vals, Bool is_call) {
  // owns env, func.
  Obj func = mem_el(vals, 0);
  assert(cmpd_len(func) == 7);
  Obj is_native = cmpd_el(func, 0);
  Obj is_macro  = cmpd_el(func, 1);
  Obj lex_env   = cmpd_el(func, 2);
  Obj variad    = cmpd_el(func, 3);
  Obj pars      = cmpd_el(func, 4);
  Obj ret_type  = cmpd_el(func, 5);
  Obj body      = cmpd_el(func, 6);
  exc_check(is_native.is_bool(), "func: %o\nis-native is not a Bool: %o", func, is_native);
  exc_check(is_macro.is_bool(), "func: %o\nis-macro is not a Bool: %o", func, is_macro);
  // TODO: check that variad is an Expr.
  exc_check(obj_is_env(lex_env), "func: %o\nenv is not an Env: %o", func, lex_env);
  exc_check(obj_type(pars) == t_Arr_Par, "func: %o\npars is not an Arr-Par: %o", func, pars);
  // TODO: check that ret-type is a Type.
  exc_check(ret_type == s_nil, "func: %o\nret-type is non-nil: %o", func, ret_type);
  if (is_call) {
    exc_check(!bool_is_true(is_macro), "cannot call macro");
  } else {
    exc_check(bool_is_true(is_macro), "cannot expand function");
  }
  Obj callee_env = env_push_frame(rc_ret(lex_env));
  // NOTE: because func is bound to self in callee_env, and func contains body,
  // we can give func to callee_env and still safely return the unretained body as tail.code.
  callee_env = run_bind_vals(d, t, callee_env, call, variad, pars, vals);
  mem_dealloc(vals);
  if (bool_is_true(is_native)) {
#if OPTION_TCO
    // caller will own env, callee_env, but not .code.
    return Step(env, callee_env, body);
#else // NO TCO.
    Step step = run(d, t, callee_env, body); // owns callee_env.
    rc_rel(step.res.env);
    return Step(env, step.res.val); // NO TCO.
#endif
  } else { // host function.
    exc_check(body.is_ptr(), "host func: %o\nbody is not a Ptr: %o", func, body);
    Func_host_ptr f_ptr = cast(Func_host_ptr, ptr_val(body));
    Obj res = f_ptr(t, callee_env);
    rc_rel(callee_env);
    return Step(env, res); // NO TCO.
  }
}


static Step run_call_accessor(Int d, Trace* t, Obj env, Obj call, Mem vals) {
  // owns env, vals.
  exc_check(vals.len == 3, "call: %o\naccessor requires 1 argument", call);
  exc_check(!mem_el(vals, 1).vld(), "call:%o\naccessee is a label", call);
  Obj accessor = mem_el_move(vals, 0);
  Obj accessee = mem_el_move(vals, 2);
  mem_dealloc(vals);
  assert(cmpd_len(accessor) == 1);
  Obj name = cmpd_el(accessor, 0);
  exc_check(name.is_sym(), "call: %o\naccessor expr is not a sym: %o", call, name);
  exc_check(obj_is_cmpd(accessee), "call: %o\naccessee is not a struct: %o", call, accessee);
  Obj type = obj_type(accessee);
  assert(obj_type(type) == t_Type);
  Obj kind = cmpd_el(type, 1);
  exc_check(is_kind_struct(kind), "call: %o\naccessee is not a struct: %o", call, accessee);
  Obj fields = cmpd_el(kind, 0);
  assert(cmpd_len(fields) == cmpd_len(accessee));
  for_in(i, cmpd_len(fields)) {
    Obj field = cmpd_el(fields, i);
    Obj field_name = cmpd_el(field, 0);
    if (field_name == name) {
      Obj val = rc_ret(cmpd_el(accessee, i));
      rc_rel(accessor);
      rc_rel(accessee);
      return Step(env, val);
    }
  }
  errFL("call: %o\naccessor field not found: %o\ntype: %o\nfields:",
    call, name, type_name(type));
  it_cmpd(it, fields) {
    errFL("  %o", cmpd_el(*it, 0));
  }
  exc_raise("");
}


static Step run_call_mutator(Int d, Trace* t, Obj env, Obj call, Mem vals) {
  // owns env, vals.
  exc_check(vals.len == 5, "call: %o\nmutator requires 2 arguments", call);
  exc_check(!mem_el(vals, 1).vld(), "call:%o\nmutatee is a label", call);
  exc_check(!mem_el(vals, 3).vld(), "call:%o\nmutator expr is a label", call);
  Obj mutator = mem_el_move(vals, 0);
  Obj mutatee = mem_el_move(vals, 2);
  Obj val = mem_el_move(vals, 4);
  mem_dealloc(vals);
  assert(cmpd_len(mutator) == 1);
  Obj name = cmpd_el(mutator, 0);
  exc_check(name.is_sym(), "call: %o\nmutator expr is not a sym: %o", call, name);
  exc_check(obj_is_cmpd(mutatee), "call: %o\nmutatee is not a struct: %o", call, mutatee);
  Obj type = obj_type(mutatee);
  assert(obj_type(type) == t_Type);
  Obj kind = cmpd_el(type, 1);
  Obj fields = cmpd_el(kind, 0);
  assert(cmpd_len(fields) == cmpd_len(mutatee));
  for_in(i, cmpd_len(fields)) {
    Obj field = cmpd_el(fields, i);
    Obj field_name = cmpd_el(field, 0);
    if (field_name == name) {
      Mem m = cmpd_mem(mutatee);
      rc_rel(m.els[i]);
      m.els[i] = val;
      rc_rel(mutator);
      return Step(env, mutatee);
    }
  }
  errFL("call: %o\nmutator field not found: %o\ntype: %o\nfields:",
    call, name, type_name(type));
  it_cmpd(it, fields) {
    errFL("  %o", cmpd_el(*it, 0));
  }
  exc_raise("");
}


static Step run_call_EXPAND(Int d, Trace* t, Obj env, Obj call, Mem vals) {
  // owns env, vals.
  exc_check(vals.len == 3, "call: %o\n:EXPAND requires 1 argument", call);
  exc_check(!mem_el(vals, 1).vld(), "call: %o\nEXPAND expr is a label", call);
  Obj callee = mem_el_move(vals, 0);
  Obj expr = mem_el_move(vals, 2);
  mem_dealloc(vals);
  rc_rel_val(callee);
  Obj res = expand(0, env, expr);
  return Step(env, res);
}


static Step run_call_RUN(Int d, Trace* t, Obj env, Obj call, Mem vals) {
  // owns env, vals.
  exc_check(vals.len == 3, "call: %o\n:RUN requires 1 argument", call);
  exc_check(!mem_el(vals, 1).vld(), "call: %o\nRUN expr is a label", call);
  Obj callee = mem_el_move(vals, 0);
  Obj expr = mem_el_move(vals, 2);
  mem_dealloc(vals);
  rc_rel_val(callee);
  // for now, perform a non-TCO run so that we do not have to retain expr.
  Step step = run(d, t, env, expr);
  rc_rel(expr);
  return step;
}


static Step run_call_CONS(Int d, Trace* t, Obj env, Obj call, Mem vals) {
  exc_check(vals.len >= 3, "call: %o\nCONS requires a type argument", call);
  exc_check(!mem_el(vals, 1).vld(), "call: %o\nCONS type argument is a label", call);
  Obj callee = mem_el_move(vals, 0);
  rc_rel_val(callee);
  Obj type = mem_el_move(vals, 2);
  Obj kind = type_kind(type);
  Int len = (vals.len - 3) / 2;
  Obj res = cmpd_new_raw(type, len);
  if (is_kind_struct(kind)) {
    Obj fields = kind_fields(kind);
    // TODO: support field defaults.
    exc_check(cmpd_len(fields) == len, "call: %o\nCONS: type %o expects %i fields; received %i",
      call, type, len, cmpd_len(fields));
    for_in(i, len) {
      Obj field = cmpd_el(fields, i);
      Obj field_name = cmpd_el(field, 0);
      Obj sym = mem_el_move(vals, i * 2 + 3);
      Obj val = mem_el_move(vals, i * 2 + 4);
      if (sym.vld()) {
        exc_check(sym == field_name, "call: %o\nCONS: expected field label: %o; received: %o",
          call, field_name, sym);
      }
      cmpd_put(res, i, val);
    }
  } else if (is_kind_arr(kind)) {
    for_in(i, len) {
      Obj sym = mem_el_move(vals, i * 2 + 3);
      Obj val = mem_el_move(vals, i * 2 + 4);
      exc_check(!sym.vld(), "call: %o\nCONS: unexpected element label: %o", call, sym);
      cmpd_put(res, i, val);
    }
  } else {
    exc_raise("call: %o\nCONS type is not a struct or arr type", call);
  }
  mem_dealloc(vals);
  return Step(env, res);
}


static Step run_Call_disp(Int d, Trace* t, Obj env, Obj code, Mem vals);
static Step run_tail(Int d, Trace* parent_trace, Step step);

static Step run_call_dispatcher(Int d, Trace* t, Obj env, Obj call, Mem vals,
  Obj dispatcher) {
  // owns env, vals.
  Obj callee = mem_el_ret(vals, 0);
  Int types_len = (vals.len - 1) / 2;
  Obj types = cmpd_new_raw(rc_ret(t_Arr_Type), types_len);
  for_in(i, types_len) {
    Obj val = mem_el(vals, i * 2 + 2);
    cmpd_put(types, i, rc_ret(obj_type(val)));
  }
  Mem disp_vals = mem_alloc(5);
  mem_put(disp_vals, 0, rc_ret(dispatcher));
  mem_put(disp_vals, 1, s_callee);
  mem_put(disp_vals, 2, callee);
  mem_put(disp_vals, 3, s_types);
  mem_put(disp_vals, 4, types);
  Step step = run_Call_disp(d, t, env, call, disp_vals);
  step = run_tail(d, t, step);
  env = step.res.env;
  // replace the original callee with the function returned by the dispatcher.
  rc_rel(mem_el_move(vals, 0));
  mem_put(vals, 0, step.res.val);
  return run_Call_disp(d, t, env, call, vals);
}


static Step run_Call(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  assert(cmpd_len(code) == 1);
  Obj exprs = cmpd_el(code, 0);
  Int len = cmpd_len(exprs);
  exc_check(len > 0, "call is empty: %o", code);
  // assemble vals as an array of interleaved name, value pairs.
  // the callee never has a name, so there are an odd number of elements.
  // the names are required for dispatch and argument binding.
  // the names are not ref-counted, but the values are.
  // the array can grow arbitrarily large due to splice arguments;
  // allocate an initial capacity sufficient for the non-splice case.
  Array vals = array_alloc_cap(len * 2 - 1);
  Step step = run(d, t, env, cmpd_el(exprs, 0)); // callee.
  env = step.res.env;
  array_append(&vals, step.res.val);
  for_imn(i, 1, len) {
    Obj expr = cmpd_el(exprs, i);
    Obj expr_t = obj_type(expr);
    if (expr_t == t_Splice) {
      Obj sub_expr = cmpd_el(expr, 0);
      step = run(d, t, env, sub_expr);
      env = step.res.env;
      Obj val = step.res.val;
      exc_check(obj_is_cmpd(val), "call: %o\nspliced value is not of a compound type: %o", val);
      it_cmpd(it, val) {
        array_append(&vals, obj0); // no name.
        array_append(&vals, rc_ret(*it));
      }
      rc_rel(val);
    } else if (expr_t == t_Label) {
      UNPACK_LABEL(expr);
      exc_check(expr_el_type == s_nil, "call: %o\nlabeled argument cannot specify a type: %o",
        code, expr_el_type);
      step = run(d, t, env, expr_el_expr);
      env = step.res.env;
      array_append(&vals, expr_el_name);
      array_append(&vals, step.res.val);
    }
    else { // unlabeled arg expr.
      step = run(d, t, env, expr);
      env = step.res.env;
      array_append(&vals, obj0); // no name.
      array_append(&vals, step.res.val);
    }
  }
  return run_Call_disp(d, t, env, code, vals.mem);
}


static Step run_Call_disp(Int d, Trace* t, Obj env, Obj code, Mem vals) {
  Obj callee = mem_el(vals, 0);
  Obj type = obj_type(callee);
  Int ti = type_index(type); // Int type avoids incomplete enum switch error.
  switch (ti) {
    case ti_Func:     return run_call_func(d, t, env, code, vals, true);
    case ti_Accessor: return run_call_accessor(d, t, env, code, vals);
    case ti_Mutator:  return run_call_mutator(d, t, env, code, vals);
    case ti_Sym:
      switch (sym_index(callee)) {
        case si_EXPAND: return run_call_EXPAND(d, t, env, code, vals);
        case si_RUN:    return run_call_RUN(d, t, env, code, vals);
        case si_CONS:   return run_call_CONS(d, t, env, code, vals);
      }
  }
  Obj kind = type_kind(type);
  if (is_kind_struct(kind)) {
    Obj dispatcher = cmpd_el(kind, 1);
    exc_check(dispatcher != s_nil, "call: %o\nobject type has nil dispatcher: %o", code, type);
    return run_call_dispatcher(d, t, env, code, vals, dispatcher);
  }
  exc_raise("call: %o\nobject is not callable: %o", code, callee);
}


// printed before each run step; white_down_pointing_small_triangle.
static const Chars trace_run_prefix = "▿ ";

// printed before each run tail step; white_right_pointing_small_triangle.
static const Chars trace_tail_prefix  = "▹ ";

// printed after each run step; white_bullet.
static const Chars trace_val_prefix = "◦ ";

// printed before each macro expand step; white_diamond.
static const Chars trace_expand_prefix = "◇ ";

// printed after each macro expand step; white_small_square.
static const Chars trace_expand_val_prefix = "▫ ";



static Step run_step_disp(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  Obj_tag ot = code.tag();
  if (ot == ot_ptr) {
    exc_raise("cannot run Ptr object: %o", code);
  }
  if (ot == ot_int) {
    return Step(env, rc_ret_val(code)); // self-evaluating.
  }
  if (ot == ot_sym) {
    if (code.is_data_word()) {
      return Step(env, rc_ret_val(code)); // self-evaluating.
    } else {
      return run_sym(t, env, code);
    }
  }
  assert(ot == ot_ref);
  Obj type = ref_type(code);
  Int ti = type_index(type); // Int type avoids incomplete enum switch error.
#define RUN(type) case ti_##type: return run_##type(d, t, env, code)
  switch (ti) {
    case ti_Data:
    case ti_Accessor:
    case ti_Mutator:
      return Step(env, rc_ret(code)); // self-evaluating.
    RUN(Quo);
    RUN(Do);
    RUN(Scope);
    RUN(Bind);
    RUN(If);
    RUN(Fn);
    RUN(Call);
  }
#undef RUN
  exc_raise("cannot run object: %o", code);
}


static Bool trace_eval = false;

static void run_err_trace(Int d, Chars p, Obj o) {
  if (trace_eval) {
    for_in(i, d) err(i % 4 ? " " : "|");
    errFL("%s %o", p, o);
  }
}


static Step run_step(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  run_err_trace(d, trace_run_prefix, code);
#if OPTION_REC_LIMIT
  exc_check(d < OPTION_REC_LIMIT, "execution exceeded recursion limit: %i", OPTION_REC_LIMIT);
#endif
  Step step = run_step_disp(d, t, env, code);
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
  Trace t(obj0, -1, parent_trace);
  do { // TCO step.
    run_err_trace(d, trace_tail_prefix, step.tail.code);
    // if the tail step is a scope or call, then a new callee environment is introduced.
    // otherwise, env and callee_env are identical, and that env was only retained once.
    Bool is_local_step = (step.tail.env == step.tail.callee_env);
    if (!is_local_step) { // step is a new scope step (scope or macro/function body).
      if (update_base_env) { // ownership transferred to base_env.
        assert(base_env == step.tail.env);
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
  if (step.res.env != base_env) {
    rc_rel(step.res.env); 
    step.res.env = base_env; // replace whatever modified callee env from TCO with the topmost env.
  }
  run_err_trace(d, trace_val_prefix, step.res.val);
  return step;
}


static Step run(Int depth, Trace* parent_trace, Obj env, Obj code) {
  // owns env.
  Int d = depth + 1;
  Trace t(code, 0, parent_trace);
  Step step = run_step(d, &t, env, code);
  return run_tail(d, &t, step);
}


static Step run_code(Obj env, Obj code) {
  // owns env.
  return run(-1, null, env, code);
}


static Obj run_macro(Trace* t, Obj env, Obj code) {
  // owns env.
  run_err_trace(0, trace_expand_prefix, code);
  assert(cmpd_len(code) == 1);
  Obj exprs = cmpd_el(code, 0);
  Int len = cmpd_len(exprs); 
  exc_check(len > 0, "expand: %o\nempty", code);
  Obj macro_sym = cmpd_el(exprs, 0);
  exc_check(macro_sym.is_sym(), "expand: %o\nargument 0 must be a Sym; found: %o",
    code, macro_sym);
  Obj macro = env_get(env, macro_sym);
  exc_check(macro.vld(), "expand: %o\nmacro lookup error: %o", code, macro_sym);
  Mem vals = mem_alloc(len * 2 - 1);
  mem_put(vals, 0, rc_ret(macro));
  for_imn(i, 1, len) {
    Obj expr = cmpd_el(exprs, i);
    mem_put(vals, i * 2 - 1, obj0);
    mem_put(vals, i * 2, rc_ret(expr));
  }
  Step step = run_call_func(0, t, env, code, vals, false); // owns env, macro.
  step = run_tail(0, t, step); // handle any TCO steps.
  rc_rel(step.res.env);
  run_err_trace(0, trace_expand_val_prefix, step.res.val);
  return step.res.val;
}


