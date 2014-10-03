// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "27-pre.h"


static Obj expand_quasiquote(Int d, Obj o) {
  // owns o.
#if OPTION_REC_LIMIT
  check(d < OPTION_REC_LIMIT, "quasiquotation exceeded recursion limit: %i\n%o",
    OPTION_REC_LIMIT, o);
#endif
  if (!obj_is_cmpd(o)) { // replace the quasiquote with quote.
    return cmpd_new1(rc_ret(t_Quo), o);
  }
  Obj type = obj_type(o);
  if (!d && is(type, t_Unq)) { // unquote is only performed at the same (innermost) level.
    check(cmpd_len(o) == 1, "malformed Unq: %o", o);
    Obj e = rc_ret(cmpd_el(o, 0));
    rc_rel(o);
    return e;
  }
  if (cmpd_contains_unquote(o)) { // unquote exists somewhere in the tree.
    Int d1 = d; // count Qua nesting level.
    if (is(type, t_Qua)) {
      d1++;
    } else if (is(type, t_Unq)) {
      d1--;
    }
    Obj s = cmpd_new_raw(rc_ret(t_Syn_struct_typed), cmpd_len(o) + 1);
    Obj* dst = cmpd_els(s);
    dst[0] = rc_ret(type_name(type));
    for_in(i, cmpd_len(o)) {
      Obj e = cmpd_el(o, i);
      dst[i + 1] = expand_quasiquote(d1, rc_ret(e)); // propagate the quotation.
    }
    rc_rel(o);
    return s;
  } else { // no unquotes in the tree; simply quote the top level.
    return cmpd_new1(rc_ret(t_Quo), o);
  }
}


static Obj run_macro(Trace* t, Obj env, Obj code);

static Obj expand(Int d, Obj env, Obj code) {
  // owns code.
#if OPTION_REC_LIMIT
  check(d < OPTION_REC_LIMIT, "macro expansion exceeded recursion limit: %i\n%o",
    OPTION_REC_LIMIT, code);
#endif
  Trace trace = {.code=code, .elided_step_count=0, .next=NULL};
  Trace* t = &trace;
  if (!obj_is_cmpd(code)) {
    return code;
  }
  Obj type = ref_type(code);
  if (is(type, t_Quo)) {
    exc_check(cmpd_len(code) == 1, "malformed Quo: %o", code);
    return code;
  }
  if (is(type, t_Qua)) {
    exc_check(cmpd_len(code) == 1, "malformed Qua: %o", code);
    Obj expr = rc_ret(cmpd_el(code, 0));
    rc_rel(code);
    return expand_quasiquote(0, expr);
  }
  if (is(type, t_Expand)) {
    Obj expanded = run_macro(t, rc_ret(env), code); // owns env.
    rc_rel(code);
    // macro result may contain more expands; recursively expand the result.
    Obj final = expand(d + 1, env, expanded);
    return final;
  } else {
    // recursively expand the elements of the struct.
    // TODO: collapse comment and VOID nodes or perhaps a special COLLAPSE node?
    // this might allow for us to do away with the preprocess phase,
    // and would also allow a macro to collapse into nothing.
    Obj expanded = cmpd_new_raw(rc_ret(ref_type(code)), cmpd_len(code));
    Obj* expanded_els = cmpd_els(expanded);
    for_in(i, cmpd_len(code)) {
      expanded_els[i] = expand(d + 1, env, rc_ret(cmpd_el(code, i)));
    }
    rc_rel(code);
    return expanded;
  }
}