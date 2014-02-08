// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "22-env.h"


static const CharsC trace_expand_prefix = "◇ ";       // white diamond
static const CharsC trace_post_expand_prefix = "▫ ";  // white small square


static Bool expr_contains_unquote(Obj o) {
  if (!obj_is_vec(o)) return false;
  Int len = vec_len(o);
  Obj* els = vec_els(o);
  assert(len > 0);
  if (els[0].u == UNQ.u) return true; // vec is an unquote form.
  for_imn(i, 1, len) {
    Obj el = els[i];
    if (expr_contains_unquote(el)) return true;
  }
  return false;
}


static Obj expr_quasiquote(Obj o) {
  // owns o.
  if (expr_contains_unquote(o)) { // implies obj_is_vec
    Int len = vec_len(o);
    Obj* src = vec_els(o);
    Obj v = new_vec_raw(len + 2);
    Obj* dst = vec_els(v);
    dst[0] = obj_ret_val(CALL);
    dst[1] = obj_ret_val(Vec);
    for_in(i, len) {
      Obj e = src[i];
      if (obj_is_vec(e) && vec_el(e, 0).u == UNQ.u) { // unquote form
        check_obj(vec_len(e) == 2, "malformed UNQ form", e);
        dst[2 + i] = obj_ret(vec_el(e, 1)); // TODO: expand?
      }
      else {
        dst[2 + i] = expr_quasiquote(obj_ret(e));
      }
    }
    obj_rel(o);
    return v;
  }
  else {
    return new_vec2(obj_ret_val(QUO), o);
  }
}


static Obj run_call_native(Obj env, Obj func, Int len, Obj* args, Bool is_expand); // owns func.

static Obj expand_macro(Obj env, Int len, Obj* args) {
  check(len > 0, "empty macro expand");
  Obj macro_sym = args[0];
  check_obj(obj_is_sym(macro_sym), "expand argument 0 must be a Sym; found", macro_sym);
  Obj macro = env_get(env, macro_sym);
  if (macro.u == obj0.u) { // lookup failed.
    error_obj("macro lookup error", macro_sym);
  }
  return run_call_native(env, obj_ret(macro), len - 1, args + 1, true);
}


static Obj expand(Obj env, Obj code) {
  // owns code.
  if (!obj_is_vec_ref(code)) {
    return code;
  }
  Int len = vec_len(code);
  Obj* els = vec_els(code);
  Obj hd = els[0];
  if (hd.u == QUO.u) {
    return code;
  }
  if (hd.u == QUA.u) {
    check_obj(len == 2, "malformed QUA form", code);
    Obj expr = obj_ret(els[1]);
    obj_rel(code);
    return expr_quasiquote(expr);
  }
  if (hd.u == EXPAND.u) {
#if VERBOSE_EVAL
    err(trace_expand_prefix); dbg(code);
#endif
      Obj expanded = expand_macro(env, len - 1, els + 1);
      obj_rel(code);
#if VERBOSE_EVAL
    err(trace_post_expand_prefix); dbg(code);
#endif
      return expand(env, expanded); // macro result may contain more expands; recursively expand.
  }
  else {
    // recursively expand vec.
    Obj expanded = new_vec_raw(len);
    Obj* expanded_els = vec_els(expanded);
    for_in(i, len) {
      expanded_els[i] = expand(env, obj_ret(els[i]));
    }
    obj_rel(code);
    return expanded;
  }
}

