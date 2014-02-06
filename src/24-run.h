// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "23-expand.h"


static Obj run_sym(Obj env, Obj code) {
  assert(obj_is_sym(code));
  assert(code.u != ILLEGAL.u); // anything that returns ILLEGAL should have raised an error.
  if (code.u < VOID.u) return obj_ret_val(code); // constants are self-evaluating.
  if (code.u == VOID.u) error("cannot run VOID");
  Obj val = env_get(env, code);
  if (val.u == ILLEGAL.u) { // lookup failed.
    error_obj("lookup error", code);
  }
  return obj_ret(val);
}


static Obj run_COMMENT(Obj env, Int len, Obj* args) {
  return obj_ret_val(VOID);
}


static Obj run_QUO(Obj env, Int len, Obj* args) {
  check(len == 1, "QUO requires 1 argument; found %ld", len);
  return obj_ret(args[0]);
}


static Obj run_DO(Obj env, Int len, Obj* args) {
  if (!len) {
    return obj_ret_val(VOID);
  }
  Int last = len - 1;
  for_in(i, last) {
    Obj o = run(env, args[i]);
    obj_rel(o); // value ignored.
  };
  return run(env, args[last]); // put last run() in tail position for TCO.
}


static Obj run_SCOPE(Obj env, Int len, Obj* args) {
  check(len == 1, "SCOPE requires 1 argument; found %ld", len);
  Obj body = args[0];
  // TODO: create new env frame.
  return run(env, body);
}


static Obj run_LET(Obj env, Int len, Obj* args) {
  check(len == 2, "LET requires 2 arguments; found %ld", len);
  Obj sym = args[0];
  Obj expr = args[1];
  check_obj(obj_is_sym(sym) && sym_is_symbol(sym), "LET requires argument 1 to be a sym; found", sym);
  Obj val = run(env, expr);
  env_bind(env, obj_ret_val(sym), val); // owns sym, val.
  return obj_ret_val(VOID); // TODO: retain and return val?
}


static Obj run_IF(Obj env, Int len, Obj* args) {
  check(len == 3, "IF requires 3 arguments; found %ld", len);
  Obj p = args[0];
  Obj t = args[1];
  Obj e = args[2];
  Obj p_val = run(env, p);
  if (is_true(p_val)) {
    obj_rel(p_val);
    return run(env, t);
  }
  else {
    obj_rel(p_val);
    return run(env, e);
  }
}


static Obj run_FN(Obj env, Int len, Obj* args) {
  check(len == 4, "FN requires 4 arguments: name:Sym is-macro:Bool parameters:Vec body:Obj; received %ld", len);
  Obj name      = args[0];
  Obj is_macro  = args[1];
  Obj pars      = args[2];
  Obj body      = args[3];
  check_obj(obj_is_sym(name),  "FN: name is not a Sym", name);
  check_obj(obj_is_bool(is_macro), "FN: is-macro is not a Bool", is_macro);
  check_obj(obj_is_vec(pars), "FN: parameters is not a Vec", pars);
  Obj f = new_vec_raw(5);
  Obj* els = vec_els(f);
  els[0] = obj_ret_val(name);
  els[1] = obj_ret_val(is_macro);
  els[2] = obj_ret(pars);
  els[3] = obj_ret(body);
  els[4] = obj_ret(env);
  return f;
}


static Obj run_call_native(Obj env, Obj func, Int len, Obj* args, Bool is_expand) {
  // owns func.
  // a native function/macro is a vec consisting of:
  //  name:Sym
  //  is-macro:Bool
  //  pars:Vec
  //  body:Expr
  //  env:Env (currently implemented as a nested Vec structure, likely to change)
  Int func_len = vec_len(func);
  check_obj(func_len == 5, "function is malformed (length is not 5)", func);
  Obj* f_els = vec_els(func);
  Obj name      = f_els[0];
  Obj is_macro  = f_els[1];
  Obj pars      = f_els[2];
  Obj body      = f_els[3];
  Obj f_env     = f_els[4];
  if (is_expand) {
    check_obj(bool_is_true(is_macro), "cannot expand function", func);
  }
  else {
    check_obj(!bool_is_true(is_macro), "cannot call macro", func);
  }
  check_obj(obj_is_sym(name),   "function is malformed (name symbol is not a Sym)", name);
  check_obj(obj_is_vec(pars),   "function is malformed (parameters is not a Vec)", pars);
  check_obj(obj_is_vec(f_env),  "function is malformed (env is not a Vec)", f_env);
  Obj frame = env_frame_bind_args(env, func, vec_len(pars), vec_els(pars), len, args, is_expand);
  Obj env1 = env_push(f_env, frame);
  Obj ret = run(env1, body);
  obj_rel(func);
  obj_rel(env1);
  return ret;
}


static Obj run_call_host(Obj env, Obj func, Int len, Obj* args) {
  // owns func.
  Func_host* fh = ref_body(func);
  Int len_pars = fh->len_pars;
  Func_host_ptr f = fh->ptr;
  // len_pars == -1 indicates a variadic function.
  check(len == len_pars || len_pars == -1, "host function expects %ld argument%s; received %ld",
    len_pars, (len_pars == 1 ? "" : "s"), len);
  obj_rel(func);
  Obj arg_vals[len]; // requires variable-length-array support from compiler.
  for_in(i, len) {
    arg_vals[i] = run(env, args[i]);
  }
  return f(len, arg_vals);
}


static Obj run_call(Obj env, Obj callee, Int len, Obj* args) {
  Obj func = run(env, callee);
  Tag ot = obj_tag(func);
  check_obj(ot == ot_ref, "object is not callable", func);
  Tag st = ref_struct_tag(func);
  switch (st) {
    case st_Vec:        return run_call_native(env, func, len, args, false);
    case st_Func_host:  return run_call_host(env, func, len, args);
    default: error_obj("object is not callable", func);
  }
}


static Obj run_Vec(Obj env, Obj code) {
  Int len = vec_len(code);
  Obj* els = vec_els(code);
  Obj form = els[0];
  Int len_args = len - 1;
  Obj* args = els + 1;
  Tag ot = obj_tag(form);
  if (ot == ot_sym) {
    Int si = sym_index(form);
#define EVAL_FORM(s) case si_##s: return run_##s(env, len_args, args)
    switch (si) {
      EVAL_FORM(COMMENT);
      EVAL_FORM(QUO);
      EVAL_FORM(DO);
      EVAL_FORM(SCOPE);
      EVAL_FORM(LET);
      EVAL_FORM(IF);
      EVAL_FORM(FN);
    }
#undef EVAL_FORM
  }
  return run_call(env, form, len_args, args);
}


// TODO: use or remove; fix comments.
static const BC trace_run_prefix    = "▿ "; // during trace, printed before each run; white_down_pointing_small_triangle.
static const BC trace_val_prefix    = "◦ "; // during trace, printed before calling continuation; white_bullet.
static const BC trace_apply_prefix  = "▹ "; // called before each call apply;  white_right_pointing_small_triangle.


static Obj run(Obj env, Obj code) {
#if VERBOSE_EVAL
    err(trace_run_prefix); dbg(code); // TODO: improve this?
#endif
  Obj_tag ot = obj_tag(code);
  if (ot & ot_flt_bit || ot == ot_int || ot == ot_data) {
    return obj_ret_val(code); // self-evaluating.
  }
  if (ot == ot_sym) {
    return run_sym(env, code);
  }
  assert_ref_is_valid(code);
  switch (ref_struct_tag(code)) {
    case st_Vec:
      return run_Vec(env, code);
    case st_Data:
    case st_I32:
    case st_I64:
    case st_U32:
    case st_U64:
    case st_F32:
    case st_F64:
      return obj_ret(code); // self-evaluating.
    case st_File:
    case st_Func_host:
    case st_Reserved_A:
    case st_Reserved_B:
    case st_Reserved_C:
    case st_Reserved_D:
    case st_Reserved_E:
    case st_Reserved_F: error_obj("cannot run object", code);
  }
}

