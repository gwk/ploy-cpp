// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "20-file.h"


typedef Obj(*Func_host_ptr)(Obj, Mem);


struct _Func_host {
  Obj type;
  Obj sym;
  Int len_pars;
  Func_host_ptr ptr;
} ALIGNED_TO_WORD;
DEF_SIZE(Func_host);

