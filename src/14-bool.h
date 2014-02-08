// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "13-sym.h"


static Obj new_bool(Int i) {
  return obj_ret_val(i ? TRUE : FALSE);
}


static Bool bool_is_true(Obj b) {
  if (b.u == TRUE.u) return true;
  if (b.u == FALSE.u) return false;
  error_obj("obj is not a Bool", b);
}


static const Obj blank;

static Bool is_true(Obj o) {
  if (obj_is_sym(o)) {
    return (sym_index(o) >= si_TRUE);
  }
#define F(c) if (o.u == c.u) return false;
  F(blank);
  F(int0);
#undef F
  return true;
}

