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

#include "17-vec.h"


struct _Env {
  Obj type;
  Obj key; // ENV_FRAME_MARKER for frame, sym for binding.
  Obj val; // src for frame, val for binding.
  Obj tl;
} ALIGNED_TO_WORD;
DEF_SIZE(Env);


static void env_rel_fields(Obj o) {
  rc_rel(o.env->key);
  rc_rel(o.env->val);
  rc_rel(o.env->tl);
}


static Obj new_env(Obj key, Obj val, Obj tl) {
  // owns sym, val, tl.
  assert(obj_is_sym(key));
  assert(tl.u == s_END.u || obj_is_env(tl));
  Obj o = ref_alloc(rt_Env, size_Env);
  o.env->type = rc_ret_val(s_ENV);
  o.env->key = key;
  o.env->val = val;
  o.env->tl = tl;
  return o;
}


static Obj env_get(Obj env, Obj sym) {
  assert(!sym_is_special(sym));
  while (env.u != s_END.u) {
    assert(obj_is_env(env));
    Obj key = env.env->key;
    if (key.u == s_ENV_FRAME_MARKER.u) { // frame marker.
      // for now, just skip the marker.
    } else { // binding.
      if (key.u == sym.u) {
        return env.env->val;
      }
    }
    env = env.env->tl;
  }
  return obj0; // lookup failed.
}


static Obj env_push_frame(Obj env, Obj src) {
  // owns env, src.
  return new_env(rc_ret_val(s_ENV_FRAME_MARKER), src, env);
}


static Obj env_bind(Obj env, Obj sym, Obj val) {
  // owns env, sym, val.
  assert(!sym_is_special(sym));
  return new_env(sym, val, env);
}


static void env_trace(Obj env, Bool show_values) {
  errL("trace:");
  while (env.u != s_END.u) {
    assert(obj_is_env(env));
    Obj key = env.env->key;
    if (key.u == s_ENV_FRAME_MARKER.u) { // frame marker.
      Obj src = env.env->val;
      err("  ");
      write_repr(stderr, src);
      err_nl();
    } else { // binding.
      if (show_values) {
        obj_err(key);
        err(" : ");
        obj_errL(env.env->val);
      }
    }
    env = env.env->tl;
  }
}
