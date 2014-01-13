// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "21-cont.h"


static Step eval(Cont cont, Obj env, Obj code);


static Step eval_sym(Cont cont, Obj env, Obj code) {
  assert(obj_is_sym(code));
  // TODO: remove this and place constants in global env?
  if (code.u < VOID.u) STEP(cont, code); // constants are self-evaluating.
  if (code.u == VOID.u) error("cannot eval VOID");
  Obj val = env_get(env, code);
  if (val.u == VOID.u) { // lookup failed.
    error_obj("lookup error", code);
  }
  STEP(cont, val);
}


static Step eval_QUO(Cont cont, Obj env, Int len, Obj* args) {
  check(len == 1, "QUO requires 1 argument; found %ld", len);
  STEP(cont, args[0]);
}


static Step eval_DO(Cont cont, Obj env, Int len, Obj* args) {
  check(len == 1, "DO requires 1 argument; found %ld", len);
  Obj body = args[0];
  if (body.u == VEC0.u) {
    STEP(cont, VOID);
  }
  check_obj(obj_is_vec(body), "DO argument must be a vec; found", body);
  body = obj_ref_borrow(body);
  Int body_len = ref_len(body);
  Obj* body_els = ref_vec_els(body);
  Cont next = cont;
  for_in_rev(i, body_len) {
    Obj a = obj_borrow(body_els[i]);
    Cont c = ^(Obj o){
      //errF("cont DO %li: ", i); obj_errL(a);
      return eval(next, env, a);
    };
    next = Block_copy(c);
  }
  STEP(next, VOID);
}


static Step eval_SCOPE(Cont cont, Obj env, Int len, Obj* args) {
  check(len == 1, "SCOPE requires 1 argument; found %ld", len);
  Obj body = args[0];
  // TODO: create new env frame.
  return eval(cont, env, body);
}


static Step eval_LET(Cont cont, Obj env, Int len, Obj* args) {
  STEP(cont, VOID);
}


static Step eval_IF(Cont cont, Obj env, Int len, Obj* args) {
  check(len == 3, "IF requires 3 arguments; found %ld", len);
  Obj pred  = obj_borrow(args[0]);
  Obj then_ = obj_borrow(args[1]);
  Obj else_ = obj_borrow(args[2]);
  Cont c = ^(Obj o){
    if (is_true(o)) {
      return eval(cont, env, then_);
    }
    else {
      return eval(cont, env, else_);
    }
  };
  return eval(Block_copy(c), env, pred);
}


static Step eval_FN(Cont cont, Obj env, Int len, Obj* args) {
  STEP(cont, VOID);
}


static Step eval_call_func(Cont cont, Obj env, Int len, Obj* args) {

}

static Step eval_bind_args(Cont cont, Obj env, Int len, Obj* args) {
  for_in_rev(i, len) {

  }
}


static Step eval_CALL(Cont cont, Obj env, Int len, Obj* args) {
  check(len > 0, "CALL requires at least one argument");
  Obj callee_expr = args[0];
  Cont c = ^(Obj callee){
    return eval_bind_args(cont, env, len, args);
  };
  return eval(c, env, callee_expr);
}


static Step eval_Vec(Cont cont, Obj env, Obj code) {
  Int len = ref_len(code);
  Obj* els = ref_vec_els(code);
  Obj form = obj_borrow(els[0]);
  Obj* args = els + 1;
  Tag ot = obj_tag(form);
  if (ot == ot_sym_data && !(form.u & data_word_bit)) {
    Int si = sym_index(form);
#define EVAL_FORM(s) case si_##s: return eval_##s(cont, env, len - 1, args)
    switch (si) {
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
  error_obj("cannot call object", code);
}


static const BC trace_eval_prefix   = "▿ "; // during trace, printed before each eval; white_down_pointing_small_triangle.
static const BC trace_cont_prefix   = "◦ "; // during trace, printed before calling continuation; white_bullet.
static const BC trace_apply_prefix  = "▹ "; // called before each call apply;  white_right_pointing_small_triangle.


static Step eval(Cont cont, Obj env, Obj code) {
  Obj_tag ot = obj_tag(code);
  if (ot & ot_flt_bit || ot == ot_int) {
    STEP(cont, code); // self-evaluating.
  }
  if (ot == ot_sym_data) {
    if (code.u & data_word_bit) {
      STEP(cont, code); // self-evaluating.
    }
    return eval_sym(cont, env, code);
  }
  if (ot == ot_reserved0 || ot == ot_reserved1) {
    error_obj("cannot eval reserved object", code);
  }
  assert_ref_is_valid(code);
  switch (ref_struct_tag(code)) {
    case st_Vec:
      return eval_Vec(cont, env, code);
    case st_Data:
    case st_I32:
    case st_I64:
    case st_U32:
    case st_U64:
    case st_F32:
    case st_F64:
      STEP(cont, code);
    case st_File:
    case st_Func_host:
    case st_Reserved_A:
    case st_Reserved_B:
    case st_Reserved_C:
    case st_Reserved_D:
    case st_Reserved_E: error_obj("cannot eval object", code);
    case st_DEALLOC:    error_obj("cannot eval deallocated object", code);
  }
}


static Obj eval_loop(Obj env, Obj code) {
  Step s = eval(NULL, env, code);
  while (s.cont) {
#if VERBOSE_EVAL
    err(trace_cont_prefix); obj_errL(s.val);
#endif
    s = s.cont(s.val);
    Block_release(s.cont);
  }
  return s.val;
}

