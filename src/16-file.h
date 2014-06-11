// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-vec.h"


typedef enum {
  ft_readable = 1 << 0,
  ft_writable = 1 << 1,
} File_tag;


static Obj new_file(CFile file, Bool is_readable, Bool is_writable) {
  Obj f = ref_alloc(st_File, size_RC + sizeof(CFile));
  CFile* p = ref_body(f);
  f.rc->mt = ((is_readable && ft_readable) | (is_writable && ft_writable));
  *p = file;
  return f;
}


static CFile file_file(Obj f) {
  assert(ref_is_file(f));
  CFile* p = ref_body(f);
  return *p;
}


UNUSED_FN static Obj file_is_readable(Obj f) {
  assert(ref_is_file(f));
  return new_bool(f.rc->mt & ft_readable);
}


UNUSED_FN static Obj file_is_writable(Obj f) {
  assert(ref_is_file(f));
  return new_bool(f.rc->mt & ft_writable);
}
