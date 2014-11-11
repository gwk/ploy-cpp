// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// ptr functions.

#include "14-dict.h"


static Obj ptr_new(Raw p) {
  Uns u = cast(Uns, p);
  assert(!(u & obj_tag_mask));
  return rc_ret_val(Obj((Uns)(u | ot_ptr)));
}


static Raw ptr_val(Obj p) {
  assert(p.is_ptr());
  return cast(Raw, (p.u & ~obj_tag_mask));
}
