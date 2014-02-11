// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// env is implemented as a chain of frames.
// a frame is a fat chain, where each link is {tl, sym, val}.

#include "21-parse.h"


static Obj env_get(Obj env, Obj sym) {
  assert(ref_is_vec(env));
  assert(sym_is_symbol(sym));
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
  return obj0; // lookup failed.
}


static Obj env_push(Obj env, Obj frame) {
  // owns env, frame.
  if (env.u == CHAIN0.u) {
    env = END;
  }
  else assert(vec_len(env) == 2);
  assert(frame.u == CHAIN0.u || vec_len(frame) == 3);
  return new_vec2(frame, env);
}


static Obj env_frame_bind(Obj frame, Obj sym, Obj val) {
  // owns frame, sym, val.
  assert(sym_is_symbol(sym));
  if (frame.u == CHAIN0.u) {
    obj_rel_val(frame);
    frame = obj_ret_val(END);
  }
  else {
    assert(vec_len(frame) == 3);
  }
  return new_vec3(sym, val, frame);
}


static void env_bind(Obj env, Obj sym, Obj val) {
  // owns sym, val.
  Obj frame = obj_ret(chain_hd(env));
  Obj frame1 = env_frame_bind(frame, sym, val);
  vec_put(env, 0, frame1);
}


static Obj env_frame_bind_args(Obj env, Obj func, Mem pars, Mem args, Bool is_expand) {
  Obj frame = obj_ret_val(CHAIN0);
  Int i_args = 0;
  Bool has_variad = false;
  for_in(i_pars, pars.len) {
    Obj par = pars.els[i_pars];
    check_obj(obj_is_par(par), "function parameter is malformed", vec_el(func, 0));
    Obj* par_els = vec_els(par);
    Obj par_kind = par_els[0]; // LABEL or VARIAD
    Obj par_sym = par_els[1];
    //Obj par_type = par_els[2];
    Obj par_expr = par_els[3];
    if (par_kind.u == LABEL.u) {
      Obj arg;
      if (i_args < args.len) {
        arg = args.els[i_args];
        i_args++;
      }
      else if (par_expr.u != NIL.u) { // TODO: what about default value of nil? is quote sufficient?
        arg = par_expr;
      }
      else {
        error_obj("function received too few arguments", vec_el(func, 0));
      }
      Obj val = (is_expand ? obj_ret(arg) : run(env, arg));
      frame = env_frame_bind(frame, obj_ret_val(par_sym), val);
    }
    else {
      assert(par_kind.u == VARIAD.u);
      check_obj(!has_variad, "function has multiple variad parameters", vec_el(func, 0));
      has_variad = true;
      Int variad_count = 0;
      for_imn(i, i_args, args.len) {
        Obj arg = args.els[i];
        if (obj_is_par(arg)) break;
        variad_count++;
      }
      Obj variad_val = new_vec_raw(variad_count);
      Obj* els = vec_els(variad_val);
      for_in(i, variad_count) {
        Obj arg = args.els[i_args++];
        els[i] = (is_expand ? obj_ret(arg) : run(env, arg));
      }
      frame = env_frame_bind(frame, obj_ret_val(par_sym), variad_val);
    }
  }
  check_obj(i_args == args.len, "function received too many arguments", vec_el(func, 0));
  return frame;
}


UNUSED_FN static void dbg_env(Obj env) {
  assert(ref_is_vec(env));
  err("env ");
  rc_err(env.rc);
  errL(":");
  while (env.u != END.u) {
    Bool first = true;
    assert(vec_len(env) == 2);
    Obj frame = chain_hd(env);
    if (frame.u != CHAIN0.u) {
      while (frame.u != END.u) {
        err(first ? "| " : "  ");
        first = false;
        assert(vec_len(frame) == 3);
        Obj key = vec_a(frame);
        Obj val = vec_b(frame);
        obj_err(key);
        err(" : ");
        obj_errL(val);
        frame = chain_tl(frame);
      }
    }
    else {
      errL("|");
    }
    env = chain_tl(env);
  }
}


