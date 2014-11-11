// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "31-run.h"


static Step eval(Obj env, Obj code) {
  Obj preprocessed = preprocess(code); // borrows code.
  if (!preprocessed.vld()) {
    return Step(env, s_void.ret_val()); // TODO: document why this is necessary.
  }
  Obj expanded = expand(0, env, preprocessed); // owns preprocessed.
  Obj compiled = compile(env, expanded); // owns expanded.
  Step step = run_code(env, compiled); // borrows compiled.
  compiled.rel();
  return step;
}


static Step eval_mem_expr(Obj env, Obj exprs) {
  // top level eval of a series of expressions.
  // this is quite different than calling eval on a Do instance;
  // besides evaluating a different, non-Expr type,
  // it also does the complete eval cycle on each item in turn.
  assert(exprs.type() == t_Arr_Expr);
  Mem m = cmpd_mem(exprs);
  if (m.len == 0) {
    return Step(env, s_void.ret_val());
  }
  Int last = m.len - 1;
  it_mem_to(it, m, last) {
    if (*it == s_HALT) {
      return Step(env, s_HALT.ret_val());
    }
    Step step = eval(env, *it);
    env = step.res.env;
    step.res.val.rel();
  }
  Step step = eval(env, m.els[last]);
  return step;
}
