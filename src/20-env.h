// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "19-parse.h"


// env is implemented as a chain of frames.
// a frame is a fat chain, where each link is {tl, sym, val}.


static Obj env_get(Obj env, Obj sym) {
  assert(ref_is_vec(env));
  assert(obj_is_sym(sym));
  while (env.u != END.u) {
    assert(vec_len(env) == 2);
    Obj frame = chain_hd(env);
    if (frame.u != CHAIN0.u) {
      while (frame.u != END.u) {
        assert(vec_len(frame) == 3);
        Obj key = vec_a(frame);
        if (key.u == sym.u) {
          return vec_b(frame);
        }
        frame = chain_tl(frame);
      }
    }
    env = chain_tl(env);
  }
  return ILLEGAL; // lookup failed.
}


static Obj env_push(Obj env, Obj frame) {
  // owns env, frame.
  assert(env.u == END.u || vec_len(env) == 2);
  assert(frame.u == CHAIN0.u || vec_len(frame) == 3);
  return new_vec2(env, frame); // note: unlike lisp, tl is in position 0.
}


static Obj env_frame_bind(Obj frame, Obj sym, Obj val) {
  // owns frame, sym, val.
  assert(obj_is_sym(sym));
  if (frame.u == CHAIN0.u) {
    obj_rel_val(frame);
    frame = obj_ret_val(END);
  }
  else {
    assert(vec_len(frame) == 3);
  }
  return new_vec3(frame, sym, val); // note: unlike lisp, tl is in position 0.
}


static void env_bind(Obj env, Obj sym, Obj val) {
  // owns sym, val.
  Obj frame = obj_ret(chain_hd(env));
  Obj frame1 = env_frame_bind(frame, sym, val);
  vec_put(env, 1, frame1); // note: unlike lisp, hd is in position 1.
}


static Obj env_frame_bind_args(Obj env, Obj func, Int len_pars, Obj* pars, Int len_args, Obj* args, Bool is_macro) {
  Obj frame = obj_ret_val(CHAIN0);
  Int i_args = 0;
  for_in(i_pars, len_pars) {
    Obj par = pars[i_pars];
    check_obj(obj_is_vec_ref(par) && vec_len(par) == 4, "function is malformed (parameter is not a Vec4)", par);
    Obj* par_els = vec_els(par);
    Obj par_kind = par_els[0]; // LABEL or VARIAD
    Obj par_sym = par_els[1];
    Obj par_type = par_els[2];
    Obj par_expr = par_els[3];
    check_obj(obj_is_sym(par_sym), "native function is malformed (parameter is not a sym)", par_sym);
    Obj arg;
    if (par_kind.u == LABEL.u) {
      if (i_args < len_args) {
        arg = args[i_args];
        i_args++;
      }
      else if (par_expr.u != NIL.u) {
        arg = par_expr;
      }
      else {
        error_obj("not enough arguments provided to function", func);
      }
      Obj val = (is_macro ? obj_ret(arg) : run(env, arg));
      frame = env_frame_bind(frame, obj_ret_val(par_sym), val);
    }
    else if (par_type.u == VARIAD.u) {
      error_obj("Variad parameters not yet supported", func);
    }
    else error_obj("native function is malformed (parameter is not a Label or Variad)", par);
  }
  return frame;
}


