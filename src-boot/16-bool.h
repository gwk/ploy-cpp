// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-sym.h"


static Obj bool_new(Int i) {
  return rc_ret_val(i ? s_true : s_false);
}


static Bool bool_is_true(Obj b) {
  if (is(b, s_true)) return true;
  if (is(b, s_false)) return false;
  error_obj("obj is not a Bool", b);
}


static const Obj blank;
static Int struct_len(Obj s);

static Bool is_true(Obj o) {
  switch (obj_tag(o)) {
    case ot_ref:
      switch (ref_tag(o)) {
        case rt_Data:
          return !is(o, blank);
        case rt_Env:
          return true;
        case rt_Struct:
          return !!struct_len(o);
      }
    case ot_ptr:
      return (ptr_val(o) != NULL);
    case ot_int:
      return (!is(o, int0));
    case ot_sym:
      return (sym_index(o) >= si_true);
  }
  assert(0);
}

