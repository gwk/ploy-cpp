// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-sym.h"


static Obj bool_new(Int i) {
  return (i ? s_true : s_false).ret_val();
}


static Bool bool_is_true(Obj b) {
  if (b == s_true) return true;
  if (b == s_false) return false;
  assert(0);
  exit(1);
}


extern const Obj blank;
static Int cmpd_len(Obj c);

static Bool is_true(Obj o) {
  switch (o.tag()) {
    case ot_ref: {
      Obj t = o.ref_type();
      if (t == t_Data) return o != blank;
      if (t == t_Env) return true;
      return !!cmpd_len(o);
    }
    case ot_ptr:
      return (ptr_val(o) != null);
    case ot_int:
      return o != int0;
    case ot_sym:
      return (sym_index(o) >= si_true);
  }
  assert(0);
}

