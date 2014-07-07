// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "27-run.h"


static Step eval(Obj env, Obj code) {
  Obj preprocessed = preprocess(code); // borrows code.
  if (preprocessed.u == obj0.u) {
    return mk_step(env, rc_ret_val(s_void)); // TODO: document why this is necessary.
  }
  Obj expanded = expand(env, preprocessed); // owns preprocessed.
  Obj compiled = compile(env, expanded); // owns expanded.
  Step step = run(env, compiled); // borrows compiled.
  rc_rel(compiled);
  return step;
}


static Step eval_struct(Obj env, Obj v) {
  // top level eval of a series of expressions.
  // this is quite different than calling eval on a DO structtor;
  // not only does this struct not have a head DO sym,
  // it also does the complete eval cycle on each member in turn.
  Mem m = struct_mem(v);
  if (m.len == 0) {
    return mk_step(env, rc_ret_val(s_void));
  }
  Int last = m.len - 1;
  it_mem_to(it, m, last) {
    Step step = eval(env, *it);
    env = step.env;
    rc_rel(step.val);
  }
  Step step = eval(env, m.els[last]);
  return step;
}
