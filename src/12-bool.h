// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "11-sym.h"


static Obj new_bool(Int i) {
  return i ? TRUE : FALSE;
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

