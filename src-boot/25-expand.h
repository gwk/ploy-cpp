// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "24-pre.h"


static const Chars_const trace_expand_prefix = "◇ "; // white diamond.
static const Chars_const trace_expand_val_prefix = "▫ "; // white small square.


static Obj expand_quasiquote(Int d, Obj o) {
  // owns o.
  if (!obj_is_struct(o)) { // replace the quasiquote with quote.
    return struct_new1(rc_ret(s_Quo), o);
  }
  Obj type = obj_type(o);
  Mem m = struct_mem(o);
  if (!d && is(type, s_Unq)) { // unquote is only performed at the same (innermost) level.
    check(m.len == 1, "malformed Unq: %o", o);
    Obj e = rc_ret(m.els[0]);
    rc_rel(o);
    return e;
  }
  if (struct_contains_unquote(o)) { // unquote exists somewhere in the tree.
    Int d1 = d; // count Qua nesting level.
    if (is(type, s_Qua)) {
      d1++;
    } else if (is(type, s_Unq)) {
      d1--;
    }
    Obj s = struct_new_raw(rc_ret(s_Syn_struct_typed), m.len + 1);
    Obj* dst = struct_els(s);
    dst[0] = rc_ret(type);
    for_in(i, m.len) {
      Obj e = m.els[i];
      dst[i + 1] = expand_quasiquote(d1, rc_ret(e)); // propagate the quotation.
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
  check(obj_is_sym(macro_sym), "expand argument 0 must be a Sym; found: %o", macro_sym);
  Obj macro = env_get(env, macro_sym);
  if (is(macro, obj0)) { // lookup failed.
    error("macro lookup error: %o", macro_sym);
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
    return expand_quasiquote(0, expr);
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
    // macro result may contain more expands; recursively expand the result.
    return expand(env, expanded);
  } else {
    // recursively expand the elements of the struct.
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
