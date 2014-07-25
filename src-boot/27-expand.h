// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "26-pre.h"


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


static Obj run_macro(Obj env, Obj code);

static Obj expand(Obj env, Obj code) {
  // owns code.
  if (!obj_is_struct(code)) {
    return code;
  }
  Mem m = struct_mem(code);
  Obj type = ref_type(code);
  if (is(type, s_Quo)) {
    exc_check(m.len == 1, "malformed Quo: %o", code);
    return code;
  }
  if (is(type, s_Qua)) {
    exc_check(m.len == 1, "malformed Qua: %o", code);
    Obj expr = rc_ret(m.els[0]);
    rc_rel(code);
    return expand_quasiquote(0, expr);
  }
  if (is(type, s_Expand)) {
    Obj expanded = run_macro(env, code);
    rc_rel(code);
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
