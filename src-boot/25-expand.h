// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "24-pre.h"


static const Chars_const trace_expand_prefix = "◇ ";       // white diamond
static const Chars_const trace_post_expand_prefix = "▫ ";  // white small square


static Obj expand_quasiquote(Obj o) {
  // owns o.
  if (!obj_is_struct(o)) { // replace the quasiquote with quote.
    return struct_new2(rc_ret(s_Quo), rc_ret_val(s_Quo), o);
  }
  Mem m = struct_mem(o);
  if (m.els[0].u == s_Unq.u) { // unquote form.
    check_obj(m.len == 2, "malformed UNQ form", o);
    Obj e = rc_ret(m.els[1]);
    rc_rel(o);
    return e;
  }
  if (struct_contains_unquote(o)) { // unquote exists somewhere in the tree.
    Obj s = struct_new_raw(rc_ret(ref_type(o)), m.len + 1);
    Obj* dst = struct_els(s);
    dst[0] = rc_ret_val(s_Syn_seq);
    for_in(i, m.len) {
      Obj e = m.els[i];
      dst[i + 1] = expand_quasiquote(rc_ret(e)); // propagate the quotation into the elements.
    }
    rc_rel(o);
    return s;
  } else { // no unquotes in the tree; simply quote the top level.
    return struct_new2(rc_ret(s_Quo), rc_ret_val(s_Quo), o);
  }
}


static Step run_call_native(Obj env, Obj func, Mem args, Bool is_expand); // owns func.
static Step run_tail(Step step);


static Obj expand_macro(Obj env, Mem args) {
  check(args.len > 0, "empty macro expand");
  Obj macro_sym = mem_el(args, 0);
  check_obj(obj_is_sym(macro_sym), "expand argument 0 must be a Sym; found", macro_sym);
  Obj macro = env_get(env, macro_sym);
  if (is(macro, obj0)) { // lookup failed.
    error_obj("macro lookup error", macro_sym);
  }
  Step step = run_call_native(rc_ret(env), rc_ret(macro), mem_next(args), true);
  step = run_tail(step);
  rc_rel(step.env);
  return step.val;
}


static Obj expand(Obj env, Obj code) {
  // owns code.
  if (!obj_is_struct(code)) {
    return code;
  }
  Mem m = struct_mem(code);
  Obj hd = m.els[0];
  if (is(hd, s_Quo)) {
    return code;
  }
  if (is(hd, s_Qua)) {
    check_obj(m.len == 2, "malformed QUA form", code);
    Obj expr = rc_ret(m.els[1]);
    rc_rel(code);
    return expand_quasiquote(expr);
  }
  if (is(hd, s_Expand)) {
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
