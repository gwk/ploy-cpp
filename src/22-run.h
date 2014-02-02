// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "21-expand.h"


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


static Obj run_VEC(Obj env, Int len, Obj* args) {
  Obj v = new_vec_raw(len);
  Obj* els = vec_els(v);
  for_in(i, len) {
    els[i] = run(env, args[i]);
  }
  return v;
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
  check_obj(obj_is_sym(sym), "LET requires argument 1 to be a sym; found", sym);
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
  check(len == 3, "FN requires 3 arguments: name-sym parameters body; received %ld", len);
  Obj sym   = args[0];
  Obj pars  = args[1];
  Obj body  = args[2];
  check_obj(obj_is_sym(sym),  "FN name is not a Sym", sym);
  check_obj(obj_is_vec(pars), "FN parameters is not a Vec", pars);
  Obj f = new_vec4(sym,
    obj_ret(pars),
    obj_ret(body),
    obj_ret(env));
  return f;
}


static Obj run_call_native(Obj env, Obj func, Int len, Obj* args, Bool is_macro) {
  // owns func.
  Int func_len = vec_len(func);
  check_obj(func_len == 4, "function is malformed (length is not 4)", func);
  Obj* func_els = vec_els(func);
  Obj f_sym = func_els[0];
  Obj pars  = func_els[1];
  Obj body  = func_els[2];
  Obj f_env = func_els[3];
  check_obj(obj_is_sym(f_sym),  "function is malformed (name symbol is not a Sym)", f_sym);
  check_obj(obj_is_vec_ref(f_env),  "function is malformed (env is not a substantial Vec)", f_env);
  check_obj(obj_is_vec(pars),   "function is malformed (parameters is not a Vec)", pars);
  Obj frame = env_frame_bind_args(env, func, vec_len(pars), vec_els(pars), len, args, is_macro);
  Obj env1 = env_push(env, frame);
  Obj ret = run(env1, body);
  obj_rel(func);
  obj_rel(env1);
  return ret;
}


static Obj run_call_host1(Obj env, Obj func, Int len, Obj* args) {
  // owns func.
  check(len == 1, "host function expects 1 argument; received %ld", len);
  Obj a0 = run(env, args[0]);
  Func_host* fh = ref_body(func);
  Func_host_ptr_1 f = cast(Func_host_ptr_1, fh->ptr);
  obj_rel(func);
  return f(a0);
}


static Obj run_call_host2(Obj env, Obj func, Int len, Obj* args) {
  // owns func.
  check(len == 2, "host function expects 2 arguments; received %ld", len);
  Obj a0 = run(env, args[0]);
  Obj a1 = run(env, args[1]);
  Func_host* fh = ref_body(func);
  Func_host_ptr_2 f = cast(Func_host_ptr_2, fh->ptr);
  obj_rel(func);
  return f(a0, a1);
}


static Obj run_call_host3(Obj env, Obj func, Int len, Obj* args) {
  // owns func.
  check(len == 2, "host function expects 3 arguments; received %ld", len);
  Obj a0 = run(env, args[0]);
  Obj a1 = run(env, args[1]);
  Obj a2 = run(env, args[2]);
  Func_host* fh = ref_body(func);
  Func_host_ptr_3 f = cast(Func_host_ptr_3, fh->ptr);
  obj_rel(func);
  return f(a0, a1, a2);
}


static Obj run_CALL(Obj env, Int len, Obj* args) {
  check(len > 0, "CALL requires at least one argument");
  Obj callee_expr = args[0];
  Obj func = run(env, callee_expr);
  Tag ot = obj_tag(func);
  check_obj(ot == ot_ref, "object is not callable", func);
  Tag st = ref_struct_tag(func);
  Int l = len - 1;
  Obj* a = args + 1;
  switch (st) {
    case st_Vec:          return run_call_native(env, func, l, a, false);
    case st_Func_host_1:  return run_call_host1(env, func, l, a);
    case st_Func_host_2:  return run_call_host2(env, func, l, a);
    case st_Func_host_3:  return run_call_host3(env, func, l, a);
    default: assert(false);
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
      EVAL_FORM(VEC);
      EVAL_FORM(QUO);
      EVAL_FORM(DO);
      EVAL_FORM(SCOPE);
      EVAL_FORM(LET);
      EVAL_FORM(IF);
      EVAL_FORM(FN);
      EVAL_FORM(CALL);
    }
#undef EVAL_FORM
  }
  error_obj("cannot call Vec object", code);
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
    case st_Func_host_1:
    case st_Func_host_2:
    case st_Func_host_3:
    case st_Reserved_A:
    case st_Reserved_B:
    case st_Reserved_C:
    case st_Reserved_D: error_obj("cannot run object", code);
  }
}

