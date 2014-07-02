// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// ptr functions.

#include "12-set.h"


static Obj ptr_new(Raw p) {
  Uns u = cast(Uns, p);
  assert(!(u & obj_tag_mask));
  return rc_ret_val((Obj){.u=(u | ot_ptr)});
}


static Raw ptr_val(Obj p) {
  assert(obj_is_ptr(p));
  return cast(Raw, (p.u & ~obj_tag_mask));
}
