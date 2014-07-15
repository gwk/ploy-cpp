// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// environments are an opaque type,
// currently implemented as a linked structure of frames and bindings;
// frames have the form: {obj0, src, tl}.
// bindings have the form: {sym, val, tl}.
// src is the object that is lexically responsible for the frame.
// this design allows env to be an immutable data structure,
// while still maintaining the notion of frame boundaries.
// frame boundaries allow us to check for redefinitions and create stack traces.

#include "18-data.h"


struct _Env {
  Obj type;
  Obj key; // ENV_FRAME_MARKER for frame, sym for binding.
  Obj val; // src for frame, val for binding.
  Obj tl;
} ALIGNED_TO_WORD;
DEF_SIZE(Env);


static Obj env_rel_fields(Obj o) {
  // returns last element for release by parent; this is a c tail call optimization.
  rc_rel(o.e->key);
  rc_rel(o.e->val);
  return o.e->tl;
}


static Obj env_new(Obj key, Obj val, Obj tl) {
  // owns sym, val, tl.
  assert(obj_is_sym(key));
  assert(is(tl, s_ENV_END_MARKER) || obj_is_env(tl));
  Obj o = ref_alloc(rt_Env, size_Env);
  o.e->type = rc_ret_val(s_Env);
  o.e->key = key;
  o.e->val = val;
  o.e->tl = tl;
  return o;
}


static Obj env_get(Obj env, Obj sym) {
  assert(!sym_is_special(sym));
  while (!is(env, s_ENV_END_MARKER)) {
    assert(obj_is_env(env));
    Obj key = env.e->key;
    if (is(key, s_ENV_FRAME_MARKER)) { // frame marker.
      // for now, just skip the marker.
    } else { // binding.
      if (is(key, sym)) {
        return env.e->val;
      }
    }
    env = env.e->tl;
  }
  return obj0; // lookup failed.
}


static Obj env_push_frame(Obj env, Obj src) {
  // owns env, src.
  return env_new(rc_ret_val(s_ENV_FRAME_MARKER), src, env);
}


static Obj env_bind(Obj env, Obj sym, Obj val) {
  // owns env, sym, val.
  assert(!sym_is_special(sym));
  Obj e = env;
  while (!is(e, s_ENV_END_MARKER)) {
    assert(obj_is_env(e));
    Obj key = e.e->key;
    if (is(key, s_ENV_FRAME_MARKER)) { // frame marker.
      break;
    } else { // binding.
      check(!is(key, sym), "symbol is already bound: %o", sym);
    }
    e = e.e->tl;
  }
  return env_new(sym, val, env);
}


static void env_trace(Obj env, Bool show_values) {
  errL("trace:");
  while (!is(env, s_ENV_END_MARKER)) {
    assert(obj_is_env(env));
    Obj key = env.e->key;
    if (is(key, s_ENV_FRAME_MARKER)) { // frame marker.
      Obj src = env.e->val;
      err("  ");
      write_repr(stderr, src);
      err_nl();
    } else { // binding.
      if (show_values) {
        errFL("%o : %o", key, env.e->val);
      }
    }
    env = env.e->tl;
  }
}
