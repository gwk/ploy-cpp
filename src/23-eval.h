// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "22-run.h"

static Obj eval(Obj env, Obj code) {
  // owns code.
  Obj expanded = expand(env, code); // owns code.
  Obj val = run(env, expanded); // does not own expanded.
  obj_rel(expanded);
  return val;
}

