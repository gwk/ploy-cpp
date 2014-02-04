// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "16-vec.h"


typedef enum {
  ft_readable = 1 << 0,
  ft_writable = 1 << 1,
} File_tag;


static Obj new_file(File file, Bool is_readable, Bool is_writable) {
  Obj f = ref_alloc(st_File, size_RC + sizeof(File));
  File* p = ref_body(f);
  f.rc->mt = ((is_readable && ft_readable) | (is_writable && ft_writable));
  *p = file;
  return f;
}


static File file_file(Obj f) {
  assert(ref_is_file(f));
  File* p = ref_body(f);
  return *p;
}


static Obj file_is_readable(Obj f) {
  assert(ref_is_file(f));
  return new_bool(f.rc->mt & ft_readable);
}


static Obj file_is_writable(Obj f) {
  assert(ref_is_file(f));
  return new_bool(f.rc->mt & ft_writable);
}


