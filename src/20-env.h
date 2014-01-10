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
