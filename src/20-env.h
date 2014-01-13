// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "19-parse.h"


static Obj env_get(Obj env, Obj sym) {
  assert_ref_is_valid(env);
  while (env.u != END.u) {
    Obj frame = vec_hd(env);
    if (frame.u != CHAIN0.u) {
      while (frame.u != END.u) {
        Obj binding = vec_hd(frame);
        assert(ref_len(binding) == 3);
        Obj key = vec_a(binding);
        if (key.u == sym.u) {
          return vec_b(binding);
        }
        frame = vec_tl(frame);
      }
    }
    env = vec_tl(env);
  }
  return VOID; // lookup failed.
}


static Obj env_cons(Obj env, Obj frame) {
  return new_vec2(frame, env);
}


static void env_bind(Obj env, Obj sym, Obj val) {
  // TODO: check for preexisting?
  Obj f0 = vec_hd(env);
  Obj f1 = new_vec3(sym, val, f0);
  vec_put(env, 0, f1);
}
