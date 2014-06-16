// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "27-run.h"

static Obj eval(Obj env, Obj code) {
  Obj preprocessed = preprocess(code); // borrows code.
  if (preprocessed.u == obj0.u) return obj_ret_val(VOID); // TODO: document why this is necessary.
  Obj expanded = expand(env, preprocessed); // owns preprocessed.
  Obj compiled = compile(env, expanded); // owns expanded.
  Obj val = run(env, compiled); // borrows compiled.
  obj_rel(compiled);
  return val;
}


static Obj eval_vec(Obj env, Obj v) {
  if (v.u == VEC0.u) return obj_ret_val(VOID);
  Mem m = vec_ref_mem(v);
  Int last = m.len - 1;
  it_mem_to(it, m, last) {
    Obj val = eval(env, *it);
    obj_rel(val);
  }
  return eval(env, m.els[last]);
}
