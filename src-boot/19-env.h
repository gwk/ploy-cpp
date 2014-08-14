// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// environments are an opaque type, currently implemented as a linked list of bindings.
// Env is an immutable data structure.
// frame boundaries are denoted by the special ENV_FRAME_MARKER symbol.
// frame boundaries allow us to check for redefinitions within a frame.

#include "18-data.h"


struct _Env {
  Ref_head head;
  Obj key; // ENV_FRAME_KEY for frame.
  Obj val; // ENV_FRAME_VAL for frame.
  Obj tl;
} ALIGNED_TO_WORD;
DEF_SIZE(Env);


static Obj env_rel_fields(Obj o) {
  // returns last element for release by parent; this is a c tail call optimization.
  rc_rel(o.e->key);
  rc_rel(o.e->val);
  return o.e->tl;
}


static Obj env_new(Bool is_mutable, Obj key, Obj val, Obj tl) {
  // owns key, val, tl.
  assert(obj_is_sym(key));
  assert(is(tl, s_ENV_END) || obj_is_env(tl));
  Obj o = ref_new(size_Env, rc_ret(t_Env), is_mutable);
  o.e->key = key;
  o.e->val = val;
  o.e->tl = tl;
  return o;
}


static Obj env_get(Obj env, Obj key) {
  assert(!sym_is_special(key));
  while (!is(env, s_ENV_END)) {
    assert(obj_is_env(env));
    if (is(env.e->key, key)) { // key is never ENV_FRAME_KEY, since marker is special.
      return env.e->val;
    }
    env = env.e->tl;
  }
  return obj0; // lookup failed.
}


static Obj env_push_frame(Obj env) {
  // owns env.
  return env_new(false, rc_ret_val(s_ENV_FRAME_KEY), rc_ret_val(s_ENV_FRAME_VAL), env);
}


static Obj env_bind(Obj env, Obj key, Obj val) {
  // owns env, key, val.
  // returns env unmodified if symbol is already bound.
  assert(!sym_is_special(key));
  Obj e = env;
  while (!is(e, s_ENV_END)) { // check that symbol is not already bound.
    assert(obj_is_env(e));
    Obj k = e.e->key;
    if (is(k, s_ENV_FRAME_KEY)) { // frame boundary; check is complete.
      break;
    } else if (is(k, key)) { // symbol is already bound.
      rc_rel(key);
      rc_rel(val);
      return env;
    }
    e = e.e->tl;
  }
  return env_new(false, key, val, env);
}

