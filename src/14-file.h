// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "13-vec.h"


static Obj new_file(File file) {
  Obj o = ref_alloc(st_File, size_RCL + sizeof(File));
  File* f = cast(File*, o.rcl + 1);
  *f = file;
  return ref_add_tag(o, ot_strong);
}

