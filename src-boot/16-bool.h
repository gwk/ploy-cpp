// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-sym.h"


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
  switch (obj_tag(o)) {
    case ot_ref:
      switch (ref_tag(o)) {
        case rt_Data:
          return o.u != blank.u;
        case rt_Env:
          return true;
        case rt_Vec:
          return !!vec_len(o);
      }
    case ot_ptr:
      return (ptr_val(o) != NULL);
    case ot_int:
      return (o.u != int0.u);
    case ot_sym:
      return (sym_index(o) >= si_true);
  }
  assert(0);
}

