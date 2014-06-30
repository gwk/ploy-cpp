// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// env is implemented as a fat chain of frames and bindings;
// each frame is a [src, tl] pair,
// and each binding is a [sym, val, tl] triple.
// src is the object that is lexically responsible for the frame.
// this design allows env to be an immutable data structure,
// while still maintaining the notion of frame boundaries.
// frame boundaries allow us to check for redefinitions and create stack traces.

#include "22-parse.h"


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


typedef struct {
  Obj caller_env;
  Obj callee_env;
} Call_envs;


static Call_envs env_bind_args(Obj caller_env, Obj callee_env, Obj func, Mem pars, Mem args, Bool is_expand) {
  // owns caller_env, callee_env.
  Int i_args = 0;
  Bool has_variad = false;
  for_in(i_pars, pars.len) {
    Obj par = pars.els[i_pars];
    check_obj(obj_is_par(par), "function parameter is malformed", vec_ref_el(func, 0));
    Obj* par_els = vec_ref_els(par);
    Obj par_kind = par_els[0]; // LABEL or VARIAD.
    Obj par_sym = par_els[1];
    //Obj par_type = par_els[2];
    Obj par_expr = par_els[3];
    if (par_kind.u == s_LABEL.u) {
      Obj arg;
      if (i_args < args.len) {
        arg = args.els[i_args];
        i_args++;
      } else if (par_expr.u != s_nil.u) { // TODO: change the default value to be unwritable?
        arg = par_expr;
      } else {
        error_obj("function received too few arguments", vec_ref_el(func, 0));
      }
      Obj val;
      if (is_expand) {
        val = rc_ret(arg);
      } else {
        Step step = run(caller_env, arg);
        caller_env = step.env;
        val = step.val;
      }
      callee_env = env_bind(callee_env, rc_ret_val(par_sym), val);
    } else {
      assert(par_kind.u == s_VARIAD.u);
      check_obj(!has_variad, "function has multiple variad parameters", vec_ref_el(func, 0));
      has_variad = true;
      Int variad_count = 0;
      for_imn(i, i_args, args.len) {
        Obj arg = args.els[i];
        if (obj_is_par(arg)) break;
        variad_count++;
      }
      Obj variad_val = new_vec_raw(variad_count);
      it_vec(it, variad_val) {
        Obj arg = args.els[i_args++];
        if (is_expand) {
          *it = rc_ret(arg);
        } else {
          Step step = run(caller_env, arg);
          caller_env = step.env;
          *it = step.val;
        }
      }
      callee_env = env_bind(callee_env, rc_ret_val(par_sym), variad_val);
    }
  }
  check_obj(i_args == args.len, "function received too many arguments", vec_ref_el(func, 0));
  return (Call_envs){.caller_env=caller_env, .callee_env=callee_env};
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
