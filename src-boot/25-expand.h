// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "24-pre.h"


static const Chars_const trace_expand_prefix = "◇ "; // white diamond.
static const Chars_const trace_expand_val_prefix = "▫ "; // white small square.


static Obj expand_quasiquote(Obj o) {
  // owns o.
  if (!obj_is_struct(o)) { // replace the quasiquote with quote.
    return struct_new1(rc_ret(s_Quo), o);
  }
  Obj type = obj_type(o);
  Mem m = struct_mem(o);
  if (is(type, s_Unq)) { // unquote form.
    check_obj(m.len == 1, "malformed UNQ form", o);
    Obj e = rc_ret(m.els[0]);
    rc_rel(o);
    return e;
  }
  if (struct_contains_unquote(o)) { // unquote exists somewhere in the tree.
    Obj s = struct_new_raw(rc_ret(type), m.len);
    Obj* dst = struct_els(s);
    for_in(i, m.len) {
      Obj e = m.els[i];
      dst[i] = expand_quasiquote(rc_ret(e)); // propagate the quotation into the elements.
    }
    rc_rel(o);
    return s;
  } else { // no unquotes in the tree; simply quote the top level.
    return struct_new1(rc_ret(s_Quo), o);
  }
}


static Step run_call_native(Int d, Obj env, Obj func, Mem args, Bool is_expand); // owns func.
static Step run_tail(Int d, Step step);


static Obj expand_macro(Obj env, Mem args) {
  check(args.len > 0, "empty macro expand");
  Obj macro_sym = mem_el(args, 0);
  check_obj(obj_is_sym(macro_sym), "expand argument 0 must be a Sym; found", macro_sym);
  Obj macro = env_get(env, macro_sym);
  if (is(macro, obj0)) { // lookup failed.
    error_obj("macro lookup error", macro_sym);
  }
  Step step = run_call_native(0, rc_ret(env), rc_ret(macro), mem_next(args), true);
  step = run_tail(0, step);
  rc_rel(step.env);
  return step.val;
}


static Obj expand(Obj env, Obj code) {
  // owns code.
  if (!obj_is_struct(code)) {
    return code;
  }
  Mem m = struct_mem(code);
  Obj type = ref_type(code);
  if (is(type, s_Quo)) {
    exc_check(m.len == 1, "malformed Quo form: %o", code);
    return code;
  }
  if (is(type, s_Qua)) {
    exc_check(m.len == 1, "malformed Qua form: %o", code);
    Obj expr = rc_ret(m.els[0]);
    rc_rel(code);
    return expand_quasiquote(expr);
  }
  if (is(type, s_Expand)) {
#if VERBOSE_EVAL
    err(trace_expand_prefix); obj_errL(code);
#endif
      Obj expanded = expand_macro(env, m);
      rc_rel(code);
#if VERBOSE_EVAL
    err(trace_expand_val_prefix); obj_errL(expanded);
#endif
    // macro result may contain more expands; recursively expand.
    return expand(env, expanded);
  } else {
    // recursively expand struct.
    // TODO: collapse comment and VOID nodes or perhaps a special COLLAPSE node?
    // this might allow for us to do away with the preprocess phase,
    // and would also allow a macro to collapse into nothing.
    Obj expanded = struct_new_raw(rc_ret(ref_type(code)), m.len);
    Obj* expanded_els = struct_els(expanded);
    for_in(i, m.len) {
      expanded_els[i] = expand(env, rc_ret(m.els[i]));
    }
    rc_rel(code);
    return expanded;
  }
}
