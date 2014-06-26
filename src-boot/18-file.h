// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "17-vec.h"


struct _File {
  Obj type;
  CFile cfile;
  Bool is_readable : 1;
  Bool is_writable : 1;
  Uns unused : width_word - 2;
} ALIGNED_TO_WORD;
DEF_SIZE(File);


static Obj new_file(CFile cfile, Bool is_readable, Bool is_writable) {
  Obj f = ref_alloc(rt_File, size_File);
  f.file->type = s_FILE;
  f.file->cfile = cfile;
  f.file->is_readable = is_readable;
  f.file->is_writable = is_writable;
  return f;
}


static CFile file_cfile(Obj f) {
  assert(ref_is_file(f));
  return f.file->cfile;
}


UNUSED_FN static Bool file_is_readable(Obj f) {
  assert(ref_is_file(f));
  return f.file->is_readable;
}


static Bool file_is_writable(Obj f) {
  assert(ref_is_file(f));
  return f.file->is_writable;
}
