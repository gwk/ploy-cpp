// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// ptr functions.

#include "12-dict.h"


static Obj ptr_new(Raw p) {
  Uns u = Uns(p);
  assert(!(u & obj_tag_mask));
  return Obj(Uns(u | ot_ptr)).ret_val();
}


static Raw ptr_val(Obj p) {
  assert(p.is_ptr());
  return Raw(p.u & ~obj_tag_mask);
}
