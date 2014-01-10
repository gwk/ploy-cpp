// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "08-array.h"


static Flt flt_val(Obj o) {
  assert(obj_tag(o) & ot_flt_bit);
  o.u &= flt_body_mask;
  return o.f;
}
