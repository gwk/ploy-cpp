// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "26-pre.h"


static Bool cmpd_contains_unquote(Obj c) {
  // TODO: follow the same depth rule as below.
  assert(c.ref_is_cmpd());
  if (c.type() == t_Unq) return true;
  Array a = cmpd_array(c);
  it_array(it, a) {
    Obj e = *it;
    if (e.is_cmpd() && cmpd_contains_unquote(e)) return true;
  }
  return false;
}


static Obj expand_quasiquote(Int qua_depth, Obj o) {
  // owns o.
  if (!o.is_cmpd()) { // replace the quasiquote with quote.
    return track_src(o, cmpd_new(t_Quo.ret(), o));
  }
  Obj type = o.type();
  if (!qua_depth && type == t_Unq) { // unquote is only performed at the same (innermost) level.
    check(cmpd_len(o) == 1, "malformed Unq: %o", o);
    Obj e = cmpd_el(o, 0).ret();
    o.rel();
    return e;
  }
  if (cmpd_contains_unquote(o)) { // unquote exists somewhere in the tree.
    Int qd1 = qua_depth; // count Qua nesting level.
    if (type == t_Qua) {
      qd1++;
    } else if (type == t_Unq) {
      qd1--;
    }
    Obj exprs = cmpd_new_raw(t_Arr_Expr.ret(), cmpd_len(o) + 2);
    cmpd_put(exprs, 0, s_CONS.ret_val());
    cmpd_put(exprs, 1, type_name(type).ret());
    for_in(i, cmpd_len(o)) {
      Obj e = cmpd_el(o, i);
      cmpd_put(exprs, i + 2, expand_quasiquote(qd1, e.ret())); // propagate the quotation.
    }
    Obj cons = track_src(o, cmpd_new(t_Call.ret(), exprs));
    o.rel();
    return cons;
  } else { // no unquotes in the tree; simply quote the top level.
    return track_src(o, cmpd_new(t_Quo.ret(), o));
  }
}


static Obj run_macro(Trace* t, Obj env, Obj code);

static Obj expand(Int d, Obj env, Obj code) {
  // owns code.
#if OPTION_REC_LIMIT
  check(d < OPTION_REC_LIMIT, "macro expansion exceeded recursion limit: %i\n%o",
    Int(OPTION_REC_LIMIT), code);
#endif
  Trace trace(code, 0, null);
  Trace* t = &trace;
  if (!code.is_cmpd()) {
    return code;
  }
  Obj type = code.ref_type();
  if (type == t_Quo) {
    exc_check(cmpd_len(code) == 1, "malformed Quo: %o", code);
    return code;
  }
  if (type == t_Qua) {
    exc_check(cmpd_len(code) == 1, "malformed Qua: %o", code);
    Obj expr = cmpd_el(code, 0).ret();
    code.rel();
    return expand_quasiquote(0, expr);
  }
  if (type == t_Expand) {
    Obj expanded = run_macro(t, env.ret(), code); // owns env.
    track_src(code, expanded);
    code.rel();
    // macro result may contain more expands; recursively expand the result.
    Obj final = expand(d + 1, env, expanded);
    return final;
  } else {
    // recursively expand the elements of the struct.
    // TODO: collapse comment and VOID nodes or perhaps a special COLLAPSE node?
    // this might allow for us to do away with the preprocess phase,
    // and would also allow a macro to collapse into nothing.
    Obj expanded = cmpd_new_raw(code.ref_type().ret(), cmpd_len(code));
    Obj* expanded_els = cmpd_els(expanded);
    for_in(i, cmpd_len(code)) {
      expanded_els[i] = expand(d + 1, env, cmpd_el(code, i).ret());
    }
    track_src(code, expanded);
    code.rel();
    return expanded;
  }
}
