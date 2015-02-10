 // Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "24-compile.h"


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
    return Step(env, code.ret_val()); // special syms are self-evaluating.
  }
  Obj val = env_get(env, code);
  exc_check(val.vld(), "lookup error: %o", code); // lookup failed.
  return Step(env, val.ret());
}


static Step run_Quo(UNUSED Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(code.cmpd_len() == 1, "Quo requires 1 field; received %i", code.cmpd_len());
  return Step(env, code.cmpd_el(0).ret());
}


static Step run(Int depth, Trace* parent_trace, Obj env, Obj code);

static Step run_Arr_Expr(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  Int len = code.cmpd_len();
  if (!len) {
    return Step(env, s_void.ret_val());
  }
  Int last = len - 1;
  for_in(i, last) {
    Step step = run(d, t, env, code.cmpd_el(i));
    env = step.res.env;
    step.res.val.rel(); // value ignored.
  };
  return Step(env, env, code.cmpd_el(last));
}


static Step run_Scope(UNUSED Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(code.cmpd_len() == 1, "Scope requires 1 field; received %i", code.cmpd_len());
  Obj expr = code.cmpd_el(0);
  Obj sub_env = env_push_frame(env.ret());
  return Step(env, sub_env, expr);
}


static Obj bind_val(Trace* t, Obj env, Bool is_public, Obj key, Obj val) {
  // owns env, key, val.
  Obj env1 = env_bind(env, is_public, key, val);
  exc_check(env1.vld(), "symbol is already bound: %o", key);
  return env1;
}


static Step run_Bind(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(code.cmpd_len() == 3, "Bind requires 3 fields; received %i", code.cmpd_len());
  Obj is_public = code.cmpd_el(0);
  Obj sym = code.cmpd_el(1);
  Obj expr = code.cmpd_el(2);
  exc_check(is_public.is_bool(), "Bind requires argument 2 to be a Bool; received: %o",
    is_public);
  exc_check(sym.is_sym(), "Bind requires argument 3 to be a bindable sym; received: %o",
    sym);
  exc_check(!sym.is_special_sym(), "Bind cannot bind to special sym: %o", sym);
  Step step = run(d, t, env, expr);
  Obj env1 = bind_val(t, step.res.env, is_public.is_true_bool(), sym.ret_val(),
    step.res.val.ret());
  return Step(env1, step.res.val);
}


static Step run_If(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(code.cmpd_len() == 3, "If requires 3 fields; received %i", code.cmpd_len());
  Obj pred = code.cmpd_el(0);
  Obj then = code.cmpd_el(1);
  Obj else_ = code.cmpd_el(2);
  Step step = run(d, t, env, pred);
  env = step.res.env;
  Obj branch = step.res.val.is_true() ? then : else_;
  step.res.val.rel();
  return Step(env, env, branch);
}


static Step run_Fn(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  exc_check(code.cmpd_len() == 4, "Fn requires 4 fields; received %i", code.cmpd_len());
  Obj is_native = code.cmpd_el(0);
  Obj is_macro  = code.cmpd_el(1);
  Obj pars_seq  = code.cmpd_el(2);
  Obj body      = code.cmpd_el(3);
  exc_check(is_macro.is_bool(), "Fn: is-macro is not a Bool: %o", is_macro);
  Bool is_macro_val = is_macro.is_true_bool();
  Obj ret_type = is_macro_val ? t_Expr : t_Obj;
  exc_check(pars_seq.type() == t_Syn_seq, "Fn: pars is not a sequence literal: %o", pars_seq);
  assert(pars_seq.cmpd_len() == 1); // single element: the array of expressions.
  Obj pars_exprs = pars_seq.cmpd_el(0);
  Int len_pars = pars_exprs.cmpd_len();
  Obj pars = Obj::Cmpd_raw(t_Arr_Par.ret(), len_pars);
  Obj variad = s_void.ret_val();
  Obj assoc = s_void.ret_val();
  for_in(i, len_pars) {
    Obj syn = pars_exprs.cmpd_el(i);
    Obj syn_type = syn.type();
    Obj par_name;
    Obj par_type_expr;
    Obj par_type;
    Obj par_dflt;
    Bool is_variad = false;
    Bool is_assoc = false;
    if (syn_type == t_Label) {
      par_name = syn.cmpd_el(0).ret();
      exc_check(par_name.is_sym(), "Fn: par name is not sym: %o", syn);
      par_type_expr = syn.cmpd_el(1);
      par_dflt = syn.cmpd_el(2).ret();
      if (is_macro_val) {
        exc_check(par_type_expr == s_nil, "Fn: macro par type must be nil:  %o", syn);
        par_type = t_Expr.ret();
      } else {
        Step step = run(d, t, env, par_type_expr);
        env = step.res.env;
        par_type = step.res.val;
        if (par_type == s_nil) {
          par_type.rel_val();
          par_type = t_Obj.ret();
        }
        exc_check(par_type.is_type(), "Fn: par expected a type: %o\nreceived: %o", syn, par_type);
      }
    } else if (syn_type == t_Variad) {
      Obj par_hd = syn.cmpd_el(0);
      par_dflt = s_void.ret_val(); // TODO: change to s_nil?
      if (par_hd.type() == t_Variad) { // nested variad indicates assoc.
        exc_check(syn.cmpd_el(1) == s_nil,
          "Fn: assoc par invalid syntax (second/nested type clause): %o", syn);
        exc_check(!is_macro_val, "Fn: macros do not support assoc pars: %o", syn);
        is_assoc = true;
        par_name = par_hd.cmpd_el(0).ret();
        exc_check(par_name.is_sym(), "Fn: par name is not sym: %o", syn);
        par_type_expr = par_hd.cmpd_el(1);
        Step step = run(d, t, env, par_type_expr);
        env = step.res.env;
        Obj el_type = step.res.val;
        exc_check(el_type.is_type(), "Fn: assoc par expected a type: %o\nreceived: %o",
          syn, el_type);
        par_type = labeled_args_type(el_type);
        el_type.rel();
      } else {
        is_variad = true;
        par_name = par_hd.ret();
        exc_check(par_name.is_sym(), "Fn: par name is not sym: %o", syn);
        par_type_expr = syn.cmpd_el(1);
        if (is_macro_val) {
          exc_check(par_type_expr == s_nil, "Fn: macro variad par type must be nil:  %o", syn);
          par_type = t_Arr_Expr.ret();
        } else {
          Step step = run(d, t, env, par_type_expr);
          env = step.res.env;
          Obj el_type = step.res.val;
          if (el_type == s_nil) {
            el_type.rel_val();
            el_type = t_Obj.ret();
          }
          exc_check(el_type.is_type(), "Fn: variad par expected a type: %o\nreceived: %o",
            syn, el_type);
          par_type = arr_type(el_type);
          el_type.rel();
        }
      }
    } else {
      exc_raise("Fn: parameter %i is neither Label nor Variad: %o", i, syn);
    }
    Obj par =  Obj::Cmpd(t_Par.ret(), par_name, par_type, par_dflt);
    pars.cmpd_put(i, par);
    if (is_variad) {
      exc_check(variad == s_void, "Fn: multiple variadic parameters: %o", syn);
      variad.rel_val();
      variad = par.ret();
    } else if (is_assoc) {
      exc_check(assoc == s_void, "Fn: multiple assoc parameters: %o", syn);
      assoc.rel_val();
      assoc = par.ret();
    }
  }
  Obj f = Obj::Cmpd(t_Func.ret(),
    is_native.ret_val(),
    is_macro.ret_val(),
    env.ret(),
    ret_type.ret(),
    variad,
    assoc,
    pars,
    body.ret());
  return Step(env, f);
}


#define UNPACK_LABEL(l) \
Obj l##_el_name = l.cmpd_el(0); \
Obj l##_el_type = l.cmpd_el(1); \
Obj l##_el_expr = l.cmpd_el(2)

#define UNPACK_VARIAD(v) \
Obj v##_el_expr = v.cmpd_el(0); \
Obj v##_el_type = v.cmpd_el(1)

#define UNPACK_PAR(p) \
Obj p##_el_name = p.cmpd_el(0); \
Obj p##_el_type = p.cmpd_el(1); \
Obj p##_el_dflt = p.cmpd_el(2)


static Obj bind_par(Int d, Trace* t, Obj env, Obj call, Obj par, Array vals, Int* i_vals) {
  // owns env.
  UNPACK_PAR(par); UNUSED_VAR(par_el_type);
  Obj val;
  Int i = *i_vals;
  if (i < vals.len()) {
    Obj arg_name = vals.el_move(i++);
    Obj arg = vals.el_move(i++);
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
  env = bind_val(t, env, false, par_el_name.ret_val(), val);
  return env;
}


static Obj bind_variad(UNUSED Int d, Trace* t, Obj env, Obj par, Array vals,
  Int* i_vals) {
  // owns env.
  UNPACK_PAR(par);
  exc_check(par_el_dflt == s_void, "variad parameter has non-void default argument: %o", par);
  Int start = *i_vals;
  Int end = vals.len();
  for_imns(i, start, end, 2) {
    if (vals.el(i).vld()) { // labeled arg; terminate variad.
      end = i;
      break;
    }
  }
  *i_vals = end;
  Int len = (end - start) / 2;
  Obj vrd = Obj::Cmpd_raw(par_el_type.ret(), len);
  Int j = 0;
  for_imns(i, start + 1, end, 2) {
    Obj arg = vals.el_move(i);
    vrd.cmpd_put(j++, arg);
  }
  env = bind_val(t, env, false, par_el_name.ret_val(), vrd);
  return env;
}


static Obj bind_assoc(UNUSED Int d, Trace* t, Obj env, Obj par, Array vals, Int start) {
  // owns env.
  UNPACK_PAR(par);
  exc_check(par_el_dflt == s_void, "assoc parameter has non-void default argument: %o", par);
  Int end = vals.len();
  Int len = (end - start) / 2;
  Obj keys = Obj::Cmpd_raw(t_Arr_Sym.ret(), len);
  Obj assoc_vals = Obj::Cmpd_raw(t_Arr_Obj.ret(), len);
  Obj assoc = Obj::Cmpd(par_el_type.ret(), keys, assoc_vals);
  Int j = 0;
  for_imns(ik, start, end, 2) {
    Int iv = ik + 1;
    Obj name = vals.el_move(ik);
    Obj arg = vals.el_move(iv);
    exc_check(name.vld(), "arg %i is not labeled", iv / 2);
    keys.cmpd_put(j, name.ret_val()); // name is not already owned.
    assoc_vals.cmpd_put(j++, arg);
  }
  env = bind_val(t,  env, false, par_el_name.ret_val(), assoc);
  return env;
}


static Obj run_bind_vals(Int d, Trace* t, Obj env, Obj call, Obj variad, Obj assoc, Obj pars,
  Array vals) {
  // owns env.
  env = bind_val(t, env, false, s_self.ret_val(), vals.el_move(0));
  Int i_vals = 1; // first name of the interleaved name/value pairs.
  Bool has_variad = false;
  for_in(i_pars, pars.cmpd_len()) {
    assert(i_vals <= vals.len() && i_vals % 2 == 1);
    Obj par = pars.cmpd_el(i_pars);
    exc_check(par.type() == t_Par, "call: %o\nparameter %i is malformed: %o",
      call, i_pars, par);
    if (par == variad) {
      exc_check(!has_variad, "call: %o\nmultiple variad parameters", call);
      has_variad = true;
      env = bind_variad(d, t, env, par, vals, &i_vals);
    } else if (par == assoc) {
      env = bind_assoc(d, t, env, par, vals, i_vals);
      return env;
    } else {
      env = bind_par(d, t, env, call, par, vals, &i_vals);
    }
  }
  exc_check(i_vals == vals.len(), "call: %o\nreceived too many arguments; bound %i; received %i",
    call, i_vals / 2, vals.len() / 2);
  return env;
}


static Step run_call_Func(Int d, Trace* t, Obj env, Obj call, Array vals, Bool is_call) {
  // owns env, func.
  Obj func = vals.el(0);
  assert(func.cmpd_len() == 8);
  Obj is_native = func.cmpd_el(0);
  Obj is_macro  = func.cmpd_el(1);
  Obj lex_env   = func.cmpd_el(2);
  Obj ret_type  = func.cmpd_el(3);
  Obj variad    = func.cmpd_el(4);
  Obj assoc     = func.cmpd_el(5);
  Obj pars      = func.cmpd_el(6);
  Obj body      = func.cmpd_el(7);
  exc_check(is_native.is_bool(), "func: %o\nis-native is not a Bool: %o", func, is_native);
  exc_check(is_macro.is_bool(), "func: %o\nis-macro is not a Bool: %o", func, is_macro);
  // TODO: check that variad is an Expr.
  exc_check(lex_env.is_env(), "func: %o\nenv is not an Env: %o", func, lex_env);
  exc_check(pars.type() == t_Arr_Par, "func: %o\npars is not an Arr-Par: %o", func, pars);
  if (is_call) {
    exc_check(!is_macro.is_true_bool(), "cannot call macro");
    exc_check(ret_type == t_Obj, "fn ret-type is not Obj: %o", ret_type); // TODO: improve.
  } else {
    exc_check(is_macro.is_true_bool(), "cannot expand function");
    exc_check(ret_type == t_Expr, "macro ret-type is not Expr: %o", ret_type);
  }
  Obj callee_env = env_push_frame(lex_env.ret());
  // NOTE: because func is bound to self in callee_env, and func contains body,
  // we can give func to callee_env and still safely return the unretained body as tail.code.
  callee_env = run_bind_vals(d, t, callee_env, call, variad, assoc, pars, vals);
  vals.dealloc();
  if (is_native.is_true_bool()) {
#if OPTION_TCO
    // caller will own env, callee_env, but not .code.
    return Step(env, callee_env, body);
#else // NO TCO.
    Step step = run(d, t, callee_env, body); // owns callee_env.
    step.res.env.rel();
    return Step(env, step.res.val); // NO TCO.
#endif
  } else { // host function.
    exc_check(body.is_ptr(), "host func: %o\nbody is not a Ptr: %o", func, body);
    Func_host_ptr f_ptr = Func_host_ptr(body.ptr());
    Obj res = f_ptr(t, callee_env);
    callee_env.rel();
    return Step(env, res); // NO TCO.
  }
}


static Step run_call_Accessor(UNUSED Int d, Trace* t, Obj env, Obj call, Array vals) {
  // owns env, vals.
  exc_check(vals.len() == 3, "call: %o\naccessor requires 1 argument", call);
  exc_check(!vals.el(1).vld(), "call:%o\naccessee is a label", call);
  Obj accessor = vals.el_move(0);
  Obj accessee = vals.el_move(2);
  vals.dealloc();
  assert(accessor.cmpd_len() == 1);
  Obj name = accessor.cmpd_el(0);
  exc_check(name.is_sym(), "call: %o\naccessor expr is not a sym: %o", call, name);
  exc_check(accessee.is_cmpd(), "call: %o\naccessee is not a struct: %o", call, accessee);
  Obj type = accessee.type();
  assert(type.type() == t_Type);
  Obj kind = type.cmpd_el(1);
  exc_check(is_kind_struct(kind), "call: %o\naccessee is not a struct: %o", call, accessee);
  Obj fields = kind.cmpd_el(0);
  assert(fields.cmpd_len() == accessee.cmpd_len());
  for_in(i, fields.cmpd_len()) {
    Obj field = fields.cmpd_el(i);
    Obj field_name = field.cmpd_el(0);
    if (field_name == name) {
      Obj val = accessee.cmpd_el(i).ret();
      accessor.rel();
      accessee.rel();
      return Step(env, val);
    }
  }
  errFL("call: %o\naccessor field not found: %o\ntype: %o\nfields:",
    call, name, type_name(type));
  for_val(e, fields.cmpd_it()) {
    errFL("  %o", e.cmpd_el(0));
  }
  exc_raise("");
}


static Step run_call_Mutator(UNUSED Int d, Trace* t, Obj env, Obj call, Array vals) {
  // owns env, vals.
  exc_check(vals.len() == 5, "call: %o\nmutator requires 2 arguments", call);
  exc_check(!vals.el(1).vld(), "call:%o\nmutatee is a label", call);
  exc_check(!vals.el(3).vld(), "call:%o\nmutator expr is a label", call);
  Obj mutator = vals.el_move(0);
  Obj mutatee = vals.el_move(2);
  Obj val = vals.el_move(4);
  vals.dealloc();
  assert(mutator.cmpd_len() == 1);
  Obj name = mutator.cmpd_el(0);
  exc_check(name.is_sym(), "call: %o\nmutator expr is not a sym: %o", call, name);
  exc_check(mutatee.is_cmpd(), "call: %o\nmutatee is not a struct: %o", call, mutatee);
  Obj type = mutatee.type();
  assert(type.type() == t_Type);
  Obj kind = type.cmpd_el(1);
  Obj fields = kind.cmpd_el(0);
  assert(fields.cmpd_len() == mutatee.cmpd_len());
  for_in(i, fields.cmpd_len()) {
    Obj field = fields.cmpd_el(i);
    Obj field_name = field.cmpd_el(0);
    if (field_name == name) {
      mutatee.cmpd_el_move(i).rel();
      mutatee.cmpd_put(i, val);
      mutator.rel();
      return Step(env, mutatee);
    }
  }
  errFL("call: %o\nmutator field not found: %o\ntype: %o\nfields:",
    call, name, type_name(type));
  for_val(e, fields.cmpd_it()) {
    errFL("  %o", e.cmpd_el(0));
  }
  exc_raise("");
}


static Step run_call_EXPAND(UNUSED Int d, Trace* t, Obj env, Obj call, Array vals) {
  // owns env, vals.
  exc_check(vals.len() == 3, "call: %o\n:EXPAND requires 1 argument", call);
  exc_check(!vals.el(1).vld(), "call: %o\nEXPAND expr is a label", call);
  Obj callee = vals.el_move(0);
  Obj expr = vals.el_move(2);
  vals.dealloc();
  callee.rel_val();
  Obj res = expand(0, env, expr);
  return Step(env, res);
}


static Step run_call_RUN(Int d, Trace* t, Obj env, Obj call, Array vals) {
  // owns env, vals.
  exc_check(vals.len() == 3, "call: %o\n:RUN requires 1 argument", call);
  exc_check(!vals.el(1).vld(), "call: %o\nRUN expr is a label", call);
  Obj callee = vals.el_move(0);
  Obj expr = vals.el_move(2);
  vals.dealloc();
  callee.rel_val();
  // for now, perform a non-TCO run so that we do not have to retain expr.
  Step step = run(d, t, env, expr);
  expr.rel();
  return step;
}


static Step run_call_CONS(UNUSED Int d, Trace* t, Obj env, Obj call, Array vals) {
  exc_check(vals.len() >= 3, "call: %o\nCONS requires a type argument", call);
  exc_check(!vals.el(1).vld(), "call: %o\nCONS type argument is a label", call);
  Obj callee = vals.el_move(0);
  callee.rel_val();
  Obj type = vals.el_move(2);
  Obj kind = type_kind(type);
  Int len = (vals.len() - 3) / 2;
  Obj res;
  if (is_kind_struct(kind)) {
    res = Obj::Cmpd_raw(type, len);
    Obj fields = kind_fields(kind);
    Int len_exp = fields.cmpd_len();
    // TODO: support field defaults.
    exc_check(len == len_exp, "call: %o\nCONS: type %o expects %i fields; received %i",
      call, type_name(type), len_exp, len);
    for_in(i, len) {
      Obj field = fields.cmpd_el(i);
      Obj field_name = field.cmpd_el(0);
      Obj sym = vals.el_move(i * 2 + 3);
      Obj val = vals.el_move(i * 2 + 4);
      if (sym.vld()) {
        exc_check(sym == field_name, "call: %o\nCONS: expected field label: %o; received: %o",
          call, field_name, sym);
      }
      res.cmpd_put(i, val);
    }
  } else if (is_kind_arr(kind)) {
    res = Obj::Cmpd_raw(type, len);
    for_in(i, len) {
      Obj sym = vals.el_move(i * 2 + 3);
      Obj val = vals.el_move(i * 2 + 4);
      exc_check(!sym.vld(), "call: %o\nCONS: unexpected element label: %o", call, sym);
      res.cmpd_put(i, val);
    }
  } else if (is_kind_unit(kind)) {
    exc_check(!len, "call: %o\nCONS: unit type expects no arguments: %o", call, type);
    res = type_unit(type);
    type.rel();
  } else {
    exc_raise("call: %o\nCONS type is not a struct, arr, or unit type", call);
  }
  vals.dealloc();
  return Step(env, res);
}


static Step run_Call_disp(Int d, Trace* t, Obj env, Obj code, Array vals);
static Step run_tail(Int d, Trace* parent_trace, Step step);

static Step run_call_dispatcher(Int d, Trace* t, Obj env, Obj call, Array vals,
  Obj dispatcher) {
  // owns env, vals.
  Obj callee = vals.el(0).ret();
  Int types_len = (vals.len() - 1) / 2;
  Obj types = Obj::Cmpd_raw(t_Arr_Type.ret(), types_len);
  for_in(i, types_len) {
    Obj val = vals.el(i * 2 + 2);
    types.cmpd_put(i, val.type().ret());
  }
  Array disp_vals(5);
  disp_vals.put(0, dispatcher.ret());
  disp_vals.put(1, s_callee);
  disp_vals.put(2, callee);
  disp_vals.put(3, s_types);
  disp_vals.put(4, types);
  Step step = run_Call_disp(d, t, env, call, disp_vals);
  step = run_tail(d, t, step);
  env = step.res.env;
  // replace the original callee with the function returned by the dispatcher.
  vals.el_move(0).rel();
  vals.put(0, step.res.val);
  return run_Call_disp(d, t, env, call, vals);
}


static Step run_Call(Int d, Trace* t, Obj env, Obj code) {
  // owns env.
  assert(code.cmpd_len() == 1);
  Obj exprs = code.cmpd_el(0);
  Int len = exprs.cmpd_len();
  exc_check(len > 0, "call is empty: %o", code);
  // assemble vals as an array of interleaved name, value pairs.
  // the callee never has a name, so there are an odd number of elements.
  // the names are required for dispatch and argument binding.
  // the names are not ref-counted, but the values are.
  // the array can grow arbitrarily large due to splice arguments;
  // allocate an initial capacity sufficient for the non-splice case.
  List vals(len * 2 - 1);
  Step step = run(d, t, env, exprs.cmpd_el(0)); // callee.
  env = step.res.env;
  vals.append(step.res.val);
  for_imn(i, 1, len) {
    Obj expr = exprs.cmpd_el(i);
    Obj expr_t = expr.type();
    if (expr_t == t_Splice) {
      Obj sub_expr = expr.cmpd_el(0);
      step = run(d, t, env, sub_expr);
      env = step.res.env;
      Obj val = step.res.val;
      exc_check(val.is_cmpd(), "call: %o\nspliced value is not of a compound type: %o", val);
      for_val(e, val.cmpd_it()) {
        vals.append(obj0); // no name.
        vals.append(e.ret());
      }
      val.rel();
    } else if (expr_t == t_Label) {
      UNPACK_LABEL(expr);
      exc_check(expr_el_type == s_nil, "call: %o\nlabeled argument cannot specify a type: %o",
        code, expr_el_type);
      step = run(d, t, env, expr_el_expr);
      env = step.res.env;
      vals.append(expr_el_name);
      vals.append(step.res.val);
    }
    else { // unlabeled arg expr.
      step = run(d, t, env, expr);
      env = step.res.env;
      vals.append(obj0); // no name.
      vals.append(step.res.val);
    }
  }
  return run_Call_disp(d, t, env, code, vals.array());
}


static Step run_Call_disp(Int d, Trace* t, Obj env, Obj code, Array vals) {
  Obj callee = vals.el(0);
  Obj type = callee.type();
  if (type == t_Func)     return run_call_Func(d, t, env, code, vals, true);
  if (type == t_Accessor) return run_call_Accessor(d, t, env, code, vals);
  if (type == t_Mutator)  return run_call_Mutator(d, t, env, code, vals);
  if (type == t_Sym) {
    switch (callee.sym_index()) {
      case si_EXPAND: return run_call_EXPAND(d, t, env, code, vals);
      case si_RUN:    return run_call_RUN(d, t, env, code, vals);
      case si_CONS:   return run_call_CONS(d, t, env, code, vals);
    }
  }
  Obj kind = type_kind(type);
  if (is_kind_struct(kind)) {
    Obj dispatcher = obj0;
    exc_raise("call: %o\ndispatchers not implemented");
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
    return Step(env, code.ret_val()); // self-evaluating.
  }
  if (ot == ot_sym) {
    if (code.is_data_word()) {
      return Step(env, code.ret_val()); // self-evaluating.
    } else {
      return run_sym(t, env, code);
    }
  }
  assert(ot == ot_ref);
  Obj type = code.ref_type();
  if (type == t_Data || type == t_Accessor || type == t_Mutator) {
    return Step(env, code.ret()); // self-evaluating.
  }
#define DISP(form) if (type == t_##form) return run_##form(d, t, env, code)
  DISP(Quo);
  DISP(Arr_Expr);
  DISP(Scope);
  DISP(Bind);
  DISP(If);
  DISP(Fn);
  DISP(Call);
#undef DISP
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
  exc_check(d < OPTION_REC_LIMIT, "execution exceeded recursion limit: %i",
    Int(OPTION_REC_LIMIT));
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
        step.tail.env.rel();
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
    step.res.env.rel(); 
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
  assert(code.cmpd_len() == 1);
  Obj exprs = code.cmpd_el(0);
  Int len = exprs.cmpd_len(); 
  exc_check(len > 0, "expand: %o\nempty", code);
  Obj macro_sym = exprs.cmpd_el(0);
  exc_check(macro_sym.is_sym(), "expand: %o\nargument 0 must be a Sym; found: %o",
    code, macro_sym);
  Obj macro = env_get(env, macro_sym);
  exc_check(macro.vld(), "expand: %o\nmacro lookup error: %o", code, macro_sym);
  Array vals(len * 2 - 1);
  vals.put(0, macro.ret());
  for_imn(i, 1, len) {
    // note: labels and splices are passed directly through as arguments;
    // otherwise we would need some sort of label 'escape' syntax.
    Obj expr = exprs.cmpd_el(i);
    vals.put(i * 2 - 1, obj0);
    vals.put(i * 2, expr.ret());
  }
  Step step = run_call_Func(0, t, env, code, vals, false); // owns env, macro.
  step = run_tail(0, t, step); // handle any TCO steps.
  step.res.env.rel();
  run_err_trace(0, trace_expand_val_prefix, step.res.val);
  return step.res.val;
}
