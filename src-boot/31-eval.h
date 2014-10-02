// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "30-run.h"


static Step eval(Obj env, Obj code) {
  //Trace t = (Trace){.call=code, .next=NULL};
  Obj preprocessed = preprocess(code); // borrows code.
  if (is(preprocessed, obj0)) {
    return mk_res(env, rc_ret_val(s_void)); // TODO: document why this is necessary.
  }
  Obj expanded = expand(0, env, preprocessed); // owns preprocessed.
  Obj compiled = compile(env, expanded); // owns expanded.
  Step step = run_code(env, compiled); // borrows compiled.
  rc_rel(compiled);
  return step;
}


static Step eval_mem_expr(Obj env, Obj exprs) {
  // top level eval of a series of expressions.
  // this is quite different than calling eval on a Do instance;
  // besides evaluating a different, non-Expr type,
  // it also does the complete eval cycle on each item in turn.
  assert(is(obj_type(exprs), t_Mem_Expr));
  Mem m = cmpd_mem(exprs);
  if (m.len == 0) {
    return mk_res(env, rc_ret_val(s_void));
  }
  Int last = m.len - 1;
  it_mem_to(it, m, last) {
    if (is(*it, s_HALT)) {
      return mk_res(env, rc_ret_val(s_HALT));
    }
    Step step = eval(env, *it);
    env = step.res.env;
    rc_rel(step.res.val);
  }
  Step step = eval(env, m.els[last]);
  return step;
}
