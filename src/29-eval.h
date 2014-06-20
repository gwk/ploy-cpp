// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "28-run.h"


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


static Step eval_vec(Obj env, Obj v) {
  // this is quite different than calling eval on a DO vector;
  // not only does this vec not have a head DO sym,
  // it also does the complete eval cycle on each member in turn.
  if (v.u == s_VEC0.u) {
    return mk_step(env, rc_ret_val(s_void));
  }
  Mem m = vec_ref_mem(v);
  Int last = m.len - 1;
  it_mem_to(it, m, last) {
    Step step = eval(env, *it);
    env = step.env;
    rc_rel(step.val);
  }
  Step step = eval(env, m.els[last]);
  return step;
}
