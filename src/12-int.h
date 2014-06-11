// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "11-set.h"


static Obj new_int(Int i) {
  check(i >= -max_Int_tagged && i <= max_Int_tagged, "large Int values not yet suppported.");
  Int shifted = i * shift_factor_Int;
  return obj_ret_val((Obj){ .i = (shifted | ot_int) });
}


static Obj new_uns(Uns u) {
  check(u < max_Uns_tagged, "large Uns values not yet supported.");
  return new_int(cast(Int, u));
}


static const Obj int0 = (Obj){.i = ot_int };

static Int int_val(Obj o) {
  assert(obj_tag(o) == ot_int);
  Int i = o.i & cast(Int, obj_body_mask);
  assert(i == 0 || i <= -shift_factor_Int || i >= shift_factor_Int);
  return i / shift_factor_Int;
}
