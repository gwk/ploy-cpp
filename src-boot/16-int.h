// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-ptr.h"


static Obj int_new(Int i) {
  check(i >= -max_Int_tagged && i <= max_Int_tagged, "large Int values not yet suppported.");
  Int shifted = i * shift_factor_Int;
  return rc_ret_val(Obj((Int)(shifted | ot_int)));
}


static Obj int_new_from_uns(Uns u) {
  check(u < max_Uns_tagged, "large Uns values not yet supported.");
  return int_new(cast(Int, u));
}


static const Obj int0 = Obj((Int)ot_int);
static const Obj int1 = Obj((Int)(ot_int + shift_factor_Int * 1));

static Int int_val(Obj o) {
  assert(obj_tag(o) == ot_int);
  Int i = o.i & cast(Int, obj_body_mask);
  assert(i == 0 || i <= -shift_factor_Int || i >= shift_factor_Int);
  return i / shift_factor_Int;
}
