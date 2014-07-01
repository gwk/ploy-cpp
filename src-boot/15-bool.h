// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "14-sym.h"


static Obj bool_new(Int i) {
  return rc_ret_val(i ? s_true : s_false);
}


static Bool bool_is_true(Obj b) {
  if (b.u == s_true.u) return true;
  if (b.u == s_false.u) return false;
  error_obj("obj is not a Bool", b);
}


static const Obj blank;

static Bool is_true(Obj o) {
  if (obj_is_sym(o)) {
    return (sym_index(o) >= si_true);
  }
#define F(c) if (o.u == c.u) return false;
  F(blank);
  F(int0);
#undef F
  return true;
}
