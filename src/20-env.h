// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "19-parse.h"


// env is implemented as a chain of frames.
// a frame is a fat chain, where each link is {tl, sym, val}.


static Obj env_get(Obj env, Obj sym) {
  assert(ref_is_vec(env));
  while (env.u != END.u) {
    assert(ref_len(env) == 2);
    Obj frame = vec_hd(env);
    if (frame.u != CHAIN0.u) {
      while (frame.u != END.u) {
        assert(ref_len(frame) == 3);
        Obj key = vec_a(frame);
        if (key.u == sym.u) {
          return vec_b(frame);
        }
        frame = vec_tl(frame);
      }
    }
    env = vec_tl(env);
  }
  return VOID; // lookup failed.
}


static Obj env_cons(Obj env, Obj frame) {
  // owns env, frame.
  return new_vec2(env, frame); // note: unlike lisp, tl is in position 0.
}


static Obj env_frame_bind(Obj frame, Obj sym, Obj val) {
  // owns frame, val.
  assert(obj_is_vec(frame) || frame.u == END.u || frame.u == CHAIN0.u);
  assert(obj_is_sym(sym));
  if (frame.u == CHAIN0.u) {
    frame = END;
  }
  return new_vec3(frame, sym, val); // note: unlike lisp, tl is in position 0.
}


static void env_bind(Obj env, Obj sym, Obj val) {
  // owns val.
  Obj frame = vec_hd(env);
  Obj frame1 = env_frame_bind(frame, sym, val);
  vec_put(env, 0, frame1);
}

