// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// environments are an opaque type, currently implemented as a linked list of bindings.
// frame boundaries are denoted by the special ENV_FRAME_MARKER symbol.
// frame boundaries allow us to check for redefinitions within a frame.

#include "16-data.h"


struct Env {
  Head head;
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
  o.e->key.rel();
  o.e->val.rel();
#if OPTION_TCO
  return o.e->tl;
#else
  o.e->tl.rel();
  return obj0;
#endif
}


static Obj env_new(Bool is_mutable, Bool is_public, Obj key, Obj val, Obj tl) {
  // owns key, val, tl.
  assert(key.is_sym());
  assert(tl == s_ENV_END || tl.is_env());
  counter_inc(ci_Env_rc);
  Obj o = Obj(raw_alloc(size_Env, ci_Env_alloc));
  o.h->type = t_Env.ret();
  o.h->rc = (1<<1) + 1;
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
  return env_new(false, false, s_ENV_FRAME_KEY.ret_val(), s_ENV_FRAME_VAL.ret_val(), env);
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
        key.rel();
        e.e->val.rel();
        e.e->val = val;
        return env;
      } else { // immutable; cannot rebind.
        env.rel();
        key.rel();
        val.rel();
        return obj0;
      }
    }
    e = e.e->tl;
  }
  return env_new(is_mutable, is_public, key, val, env);
}


static Obj global_env;


static void env_init() {
  global_env = s_ENV_END.ret_val();
}


#if OPTION_ALLOC_COUNT
static void env_cleanup() {
  global_env.rel();
  global_env = obj0;
}
#endif


