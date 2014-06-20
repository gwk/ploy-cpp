// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "25-pre.h"


static const Chars_const trace_expand_prefix = "◇ ";       // white diamond
static const Chars_const trace_post_expand_prefix = "▫ ";  // white small square


static Bool expr_contains_unquote(Obj o) {
  if (!obj_is_vec_ref(o)) return false;
  Mem m = vec_ref_mem(o);
  assert(m.len > 0);
  if (m.els[0].u == s_UNQ.u) return true; // vec is an unquote form.
  it_mem_from(it, m, 1) {
    if (expr_contains_unquote(*it)) return true;
  }
  return false;
}


static Obj expr_quasiquote(Obj o) {
  // owns o.
  if (expr_contains_unquote(o)) { // implies obj_is_vec_ref.
    Mem m = vec_ref_mem(o);
    Obj v = new_vec_raw(m.len + 1);
    Obj* dst = vec_ref_els(v);
    dst[0] = rc_ret_val(s_SEQ);
    for_in(i, m.len) {
      Obj e = m.els[i];
      if (obj_is_vec_ref(e) && vec_ref_el(e, 0).u == s_UNQ.u) { // unquote form
        check_obj(vec_ref_len(e) == 2, "malformed s_UNQ form", e);
        dst[1 + i] = rc_ret(vec_ref_el(e, 1)); // TODO: expand?
      } else {
        dst[1 + i] = expr_quasiquote(rc_ret(e));
      }
    }
    rc_rel(o);
    return v;
  } else {
    return new_vec2(rc_ret_val(s_QUO), o);
  }
}


static Step run_call_native(Obj env, Obj func, Mem args, Bool is_expand); // owns func.

static Obj expand_macro(Obj env, Mem args) {
  check(args.len > 0, "empty macro expand");
  Obj macro_sym = mem_el(args, 0);
  check_obj(obj_is_sym(macro_sym), "expand argument 0 must be a Sym; found", macro_sym);
  Obj macro = env_get(env, macro_sym);
  if (macro.u == obj0.u) { // lookup failed.
    error_obj("macro lookup error", macro_sym);
  }
  Step step = run_call_native(rc_ret(env), rc_ret(macro), mem_next(args), true);
  rc_rel(step.env);
  return step.obj;
}


static Obj expand(Obj env, Obj code) {
  // owns code.
  if (!obj_is_vec_ref(code)) {
    return code;
  }
  Mem m = vec_ref_mem(code);
  Obj hd = m.els[0];
  if (hd.u == s_QUO.u) {
    return code;
  }
  if (hd.u == s_QUA.u) {
    check_obj(m.len == 2, "malformed s_QUA form", code);
    Obj expr = rc_ret(m.els[1]);
    rc_rel(code);
    return expr_quasiquote(expr);
  }
  if (hd.u == s_EXPAND.u) {
#if VERBOSE_EVAL
    err(trace_expand_prefix); dbg(code);
#endif
      Obj expanded = expand_macro(env, mem_next(m));
      rc_rel(code);
#if VERBOSE_EVAL
    err(trace_post_expand_prefix); dbg(code);
#endif
      return expand(env, expanded); // macro result may contain more expands; recursively expand.
  } else {
    // recursively expand vec.
    Obj expanded = new_vec_raw(m.len);
    Obj* expanded_els = vec_els(expanded);
    for_in(i, m.len) {
      expanded_els[i] = expand(env, rc_ret(m.els[i]));
    }
    rc_rel(code);
    return expanded;
  }
}