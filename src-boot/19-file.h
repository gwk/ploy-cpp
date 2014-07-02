// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-env.h"


struct _File {
  Obj type;
  CFile cfile;
} ALIGNED_TO_WORD;
DEF_SIZE(File);


static Obj file_new(CFile cfile) {
  Obj f = ref_alloc(rt_File, size_File);
  f.file->type = rc_ret_val(s_File);
  f.file->cfile = cfile;
  return f;
}


static CFile file_cfile(Obj f) {
  assert(ref_is_file(f));
  return f.file->cfile;
}

