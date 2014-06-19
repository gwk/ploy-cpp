// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "17-vec.h"


typedef struct {
  CFile cfile;
  Bool is_readable : 1;
  Bool is_writable : 1;
  Uns unused : width_word - 2;
} ALIGNED_TO_WORD File;


static Obj new_file(CFile cfile, Bool is_readable, Bool is_writable) {
  Obj f = ref_alloc(st_File, size_RH + sizeof(File));
  File* b = ref_body(f);
  b->cfile = cfile;
  b->is_readable = is_readable;
  b->is_writable = is_writable;
  return f;
}


static CFile file_cfile(Obj f) {
  assert(ref_is_file(f));
  File* b = ref_body(f);
  return b->cfile;
}


UNUSED_FN static Bool file_is_readable(Obj f) {
  assert(ref_is_file(f));
  File* b = ref_body(f);
  return b->is_readable;
}


static Bool file_is_writable(Obj f) {
  assert(ref_is_file(f));
  File* b = ref_body(f);
  return b->is_writable;
}
