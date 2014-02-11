// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "25-run.h"

static Obj eval(Obj env, Obj code) {
  Obj preprocessed = preprocess(code); // borrows code.
  if (preprocessed.u == obj0.u) return obj_ret_val(VOID);
  Obj expanded = expand(env, preprocessed); // owns preprocessed.
  Obj val = run(env, expanded); // borrows expanded.
  obj_rel(expanded);
  return val;
}


static Obj eval_vec(Obj env, Obj v) {
  if (v.u == VEC0.u) return obj_ret_val(VOID);
  Mem m = vec_mem(v);
  Int last = m.len - 1;
  it_mem_to(it, m, last) {
    Obj val = eval(env, *it);
    obj_rel(val);
  }
  return eval(env, m.els[last]);
}

