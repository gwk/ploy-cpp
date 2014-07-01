// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// env is implemented as a fat chain of frames and bindings;
// each frame is a [src, tl] pair,
// and each binding is a [sym, val, tl] triple.
// src is the object that is lexically responsible for the frame.
// this design allows env to be an immutable data structure,
// while still maintaining the notion of frame boundaries.
// frame boundaries allow us to check for redefinitions and create stack traces.

#include "17-vec.h"


static Obj env_get(Obj env, Obj sym) {
  assert(ref_is_vec(env));
  assert(!sym_is_special(sym));
  while (env.u != s_END.u) {
    Int len = vec_ref_len(env);
    if (len == 2) { // frame marker.
      // for now, just skip the marker.
    } else { // binding.
      assert(len == 3);
      Obj key = chain_a(env);
      if (key.u == sym.u) {
        return chain_b(env);
      }
    }
    env = chain_tl(env);
  }
  return obj0; // lookup failed.
}


static Obj env_add_frame(Obj env, Obj src) {
  // owns env, src.
  return new_vec2(src, env);
}


static Obj env_bind(Obj env, Obj sym, Obj val) {
  // env, sym, val.
  assert(!sym_is_special(sym));
  assert(obj_is_vec(env) || env.u == s_END.u);
  return new_vec3(sym, val, env);
}


static void env_trace(Obj env, Bool show_values) {
  assert(ref_is_vec(env));
  errL("trace:");
  while (env.u != s_END.u) {
    Int len = vec_ref_len(env);
    if (len == 2) { // frame marker.
      Obj src = chain_a(env);
      err("  ");
      write_repr(stderr, src);
      err_nl();
    } else { // binding.
      assert(vec_ref_len(env) == 3);
      if (show_values) {
        Obj key = chain_a(env);
        Obj val = chain_b(env);
        obj_err(key);
        err(" : ");
        obj_errL(val);
      }
    }
    env = chain_tl(env);
  }
}
