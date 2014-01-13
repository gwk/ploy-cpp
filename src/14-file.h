// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "13-vec.h"


static Obj new_file(File file) {
  Obj f = ref_alloc(st_File, size_RC + sizeof(File));
  File* p = ref_body(f);
  *p = file;
  return ref_add_tag(f, ot_strong);
}


static File file_file(Obj f) {
  assert(ref_is_file(f));
  File* p = ref_body(f);
  return *p;
}

