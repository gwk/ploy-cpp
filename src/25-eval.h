// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "24-run.h"

static Obj eval(Obj env, Obj code) {
  Obj expanded = expand(env, obj_ret(code)); // owns code.
  Obj val = run(env, expanded); // does not own expanded.
  obj_rel(expanded);
  return val;
}


static Obj eval_vec(Obj env, Obj v) {
  if (v.u == VEC0.u) return VOID;
  Int len = vec_len(v);
  Obj* els = vec_els(v);
  for_in(i, len - 1) {
    Obj val = eval(env, els[i]);
    obj_rel(val);
  }
  return eval(env, els[len - 1]);
}

