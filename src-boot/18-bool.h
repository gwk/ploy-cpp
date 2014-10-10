// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "17-sym.h"


static Obj bool_new(Int i) {
  return rc_ret_val(i ? s_true : s_false);
}


static Bool bool_is_true(Obj b) {
  if (is(b, s_true)) return true;
  if (is(b, s_false)) return false;
  error("obj is not a Bool: %o", b);
}


static const Obj blank;
static Int cmpd_len(Obj c);

static Bool is_true(Obj o) {
  switch (obj_tag(o)) {
    case ot_ref: {
      Obj t = ref_type(o);
      if (is(t, t_Data)) return !is(o, blank);
      if (is(t, t_Env)) return true;
      return !!cmpd_len(o);
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

