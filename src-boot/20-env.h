// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// environments are an opaque type, currently implemented as a linked list of bindings.
// frame boundaries are denoted by the special ENV_FRAME_MARKER symbol.
// frame boundaries allow us to check for redefinitions within a frame.

#include "19-data.h"


struct Env {
  Ref_head head;
  Bool is_mutable: 1;
  Bool is_public : 1;
  Uns bit_padding : 6;
  Char padding[7];
  Obj key; // ENV_FRAME_KEY for frame.
  Obj val; // ENV_FRAME_VAL for frame.
  Obj tl;
} ALIGNED_TO_WORD;
DEF_SIZE(Env);


static Obj env_rel_fields(Obj o) {
  // returns last element for release by parent; this is a c tail call optimization.
  rc_rel(o.e->key);
  rc_rel(o.e->val);
#if OPTION_TCO
  return o.e->tl;
#else
  rc_rel(o.e->tl);
  return obj0;
#endif
}


static Obj env_new(Bool is_mutable, Bool is_public, Obj key, Obj val, Obj tl) {
  // owns key, val, tl.
  assert(key.is_sym());
  assert(tl == s_ENV_END || tl.is_env());
  Obj o = ref_new(size_Env, rc_ret(t_Env));
  o.e->is_mutable = is_mutable;
  o.e->is_public = is_public;
  o.e->key = key;
  o.e->val = val;
  o.e->tl = tl;
  return o;
}


static Obj env_get(Obj env, Obj key) {
  assert(!sym_is_special(key));
  while (env != s_ENV_END) {
    assert(env.is_env());
    if (env.e->key == key) { // key is never ENV_FRAME_KEY, since marker is special.
      return env.e->val;
    }
    env = env.e->tl;
  }
  return obj0; // lookup failed.
}


static Obj env_push_frame(Obj env) {
  // owns env.
  return env_new(false, false, rc_ret_val(s_ENV_FRAME_KEY), rc_ret_val(s_ENV_FRAME_VAL), env);
}


static Obj env_bind(Obj env, Bool is_mutable, Bool is_public, Obj key, Obj val) {
  // owns env, key, val.
  // returns obj0 on failure.
  assert(!sym_is_special(key));
  Obj e = env;
  while (e != s_ENV_END) { // check that symbol is not already bound.
    assert(e.is_env());
    Obj k = e.e->key;
    if (!is_mutable && k == s_ENV_FRAME_KEY) { // frame boundary; check is complete.
      break;
    } else if (k == key) { // symbol is already bound.
      if (is_mutable && e.e->is_mutable) { // mutate the mutable binding.
        rc_rel(key);
        rc_rel(e.e->val);
        e.e->val = val;
        return env;
      } else { // immutable; cannot rebind.
        rc_rel(env);
        rc_rel(key);
        rc_rel(val);
        return obj0;
      }
    }
    e = e.e->tl;
  }
  return env_new(is_mutable, is_public, key, val, env);
}


static Obj global_env;


static void env_init() {
  global_env = rc_ret_val(s_ENV_END);
}


#if OPTION_ALLOC_COUNT
static void env_cleanup() {
  rc_rel(global_env);
  global_env = obj0;
}
#endif


