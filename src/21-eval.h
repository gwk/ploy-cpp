// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "20-env.h"


static Obj eval(Obj env, Obj code);


static Obj eval_sym(Obj env, Obj code) {
  assert(obj_is_sym(code));
  // TODO: remove this and place constants in global env?
  if (code.u < VOID.u) return code; // constants are self-evaluating.
  if (code.u == VOID.u) error("cannot eval VOID");
  Obj val = env_get(env, code);
  if (val.u == VOID.u) { // lookup failed.
    error_obj("lookup error", code);
  }
  return obj_retain_strong(val);
}


static Obj eval_QUO(Obj env, Int len, Obj* args) {
  check(len == 1, "QUO requires 1 argument; found %ld", len);
  return obj_retain_strong(args[0]);
}


static Obj eval_DO(Obj env, Int len, Obj* args) {
  if (!len) {
    return VOID;
  }
  Int last = len - 1;
  for_in(i, last) {
    Obj o = eval(env, args[i]);
    obj_release_strong(o); // value ignored.
  };
  return eval(env, args[last]); // put last eval in tail position for optimization.
}


static Obj eval_SCOPE(Obj env, Int len, Obj* args) {
  check(len == 1, "SCOPE requires 1 argument; found %ld", len);
  Obj body = args[0];
  // TODO: create new env frame.
  return eval(env, body);
}


static Obj eval_LET(Obj env, Int len, Obj* args) {
  check(len == 2, "LET requires 2 arguments; found %ld", len);
  Obj sym = args[0];
  Obj expr = args[1];
  check_obj(obj_is_sym(sym), "LET requires argument 1 to be a sym; found", sym);
  Obj val = eval(env, expr);
  env_bind(env, sym, val); // owns val.
  return VOID;
}


static Obj eval_IF(Obj env, Int len, Obj* args) {
  check(len == 3, "IF requires 3 arguments; found %ld", len);
  Obj p = args[0];
  Obj t = args[1];
  Obj e = args[2];
  Obj p_val = eval(env, p);
  if (is_true(p_val)) {
    obj_release_strong(p_val);
    return eval(env, t);
  }
  else {
    obj_release_strong(p_val);
    return eval(env, e);
  }
}


static Obj eval_FN(Obj env, Int len, Obj* args) {
  return VOID;
}


static Obj eval_call_native(Obj env, Obj func, Int len, Obj* args) {
  Int func_len = ref_len(func);
  // convert to assert pending parse verification?
  check_obj(func_len == 2, "native function is malformed (length is not 2)", func);
  Obj* func_els = vec_els(func);
  Obj pars = func_els[0];
  Obj body = func_els[1];
  // convert to assert pending parse verification?
  check_obj(obj_is_vec(pars), "native function is malformed (parameters is not a vec)", func);
  Int pars_len = ref_len(pars);
  check(pars_len == len, "native function expects %ld argument; received %ld",  pars_len, len);
  Obj frame;
  if (len) {
    frame = END;
    for_in(i, len) {
      Obj sym = vec_el(pars, i);
      Obj expr = args[i];
      Obj val = eval(env, expr);
      // convert to assert pending parse verification?
      check_obj(obj_is_sym(sym), "native function is malformed (parameter is not a sym)", sym);
      frame = env_frame_bind(frame, sym, val);
    }
  }
  else {
    frame = CHAIN0;
  }
  Obj env1 = env_cons(env, frame);
  Obj ret = eval(env1, body);
  obj_release_strong(env1);
  return ret;
}


static Obj eval_call_host1(Obj env, Obj func, Int len, Obj* args) {
  check(len == 1, "host function expects 1 argument; received %ld", len);
  Obj a0 = eval(env, args[0]);
  Func_host* fh = ref_body(func);
  Func_host_ptr_1 f = cast(Func_host_ptr_1, fh->ptr);
  Obj ret = f(a0);
  obj_release_strong(a0);
  return ret;
}


static Obj eval_call_host2(Obj env, Obj func, Int len, Obj* args) {
  check(len == 2, "host function expects 2 arguments; received %ld", len);
  Obj a0 = eval(env, args[0]);
  Obj a1 = eval(env, args[1]);
  Func_host* fh = ref_body(func);
  Func_host_ptr_2 f = cast(Func_host_ptr_2, fh->ptr);
  Obj ret = f(a0, a1);
  obj_release_strong(a0);
  obj_release_strong(a1);
  return ret;
}


static Obj eval_call_host3(Obj env, Obj func, Int len, Obj* args) {
  check(len == 2, "host function expects 3 arguments; received %ld", len);
  Obj a0 = eval(env, args[0]);
  Obj a1 = eval(env, args[1]);
  Obj a2 = eval(env, args[2]);
  Func_host* fh = ref_body(func);
  Func_host_ptr_3 f = cast(Func_host_ptr_3, fh->ptr);
  Obj ret = f(a0, a1, a2);
  obj_release_strong(a0);
  obj_release_strong(a1);
  obj_release_strong(a2);
  return ret;
}


static Obj eval_CALL(Obj env, Int len, Obj* args) {
  check(len > 0, "CALL requires at least one argument");
  Obj callee_expr = args[0];
  Obj func = eval(env, callee_expr);
  Tag ot = obj_tag(func);
  check_obj(ot == ot_ref, "object is not callable", func);
  Tag st = ref_struct_tag(func);
  Int l = len - 1;
  Obj* a = args + 1;
  switch (st) {
    case st_Vec:          return eval_call_native(env, func, l, a);
    case st_Func_host_1:  return eval_call_host1(env, func, l, a);
    case st_Func_host_2:  return eval_call_host2(env, func, l, a);
    case st_Func_host_3:  return eval_call_host3(env, func, l, a);
    default: assert(false);
  }
}


static Obj eval_COMMENT(Obj env, Int len, Obj* args) {
  return VOID;
}


static Obj eval_Vec(Obj env, Obj code) {
  Int len = ref_len(code);
  Obj* els = vec_els(code);
  Obj form = els[0];
  Int len_args = len - 1;
  Obj* args = els + 1;
  Tag ot = obj_tag(form);
  if (ot == ot_sym) {
    Int si = sym_index(form);
#define EVAL_FORM(s) case si_##s: return eval_##s(env, len_args, args)
    switch (si) {
      EVAL_FORM(QUO);
      EVAL_FORM(DO);
      EVAL_FORM(SCOPE);
      EVAL_FORM(LET);
      EVAL_FORM(IF);
      EVAL_FORM(FN);
      EVAL_FORM(CALL);
      EVAL_FORM(COMMENT);
    }
#undef EVAL_FORM
  }
  error_obj("cannot call Vec object", code);
}


static const BC trace_eval_prefix   = "▿ "; // during trace, printed before each eval; white_down_pointing_small_triangle.
static const BC trace_cont_prefix   = "◦ "; // during trace, printed before calling continuation; white_bullet.
static const BC trace_apply_prefix  = "▹ "; // called before each call apply;  white_right_pointing_small_triangle.


static Obj eval(Obj env, Obj code) {
#if VERBOSE_EVAL
    err(trace_cont_prefix); dbg(code); // TODO: improve this?
#endif
  Obj_tag ot = obj_tag(code);
  if (ot & ot_flt_bit || ot == ot_int || ot == ot_data) {
    return code; // self-evaluating.
  }
  if (ot == ot_sym) {
    return eval_sym(env, code);
  }
  assert_ref_is_valid(code);
  switch (ref_struct_tag(code)) {
    case st_Vec:
      return eval_Vec(env, code);
    case st_Data:
    case st_I32:
    case st_I64:
    case st_U32:
    case st_U64:
    case st_F32:
    case st_F64:
      return obj_retain_strong(code); // self-evaluating.
    case st_File:
    case st_Func_host_1:
    case st_Func_host_2:
    case st_Func_host_3:
    case st_Reserved_A:
    case st_Reserved_B:
    case st_Reserved_C: error_obj("cannot eval object", code);
    case st_DEALLOC:    error_obj("cannot eval deallocated object", code);
  }
}

