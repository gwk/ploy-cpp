// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "17-file.h"


typedef Obj(*Func_host_ptr)(Mem);


typedef struct {
  Obj sym;
  Int len_pars;
  Func_host_ptr ptr;
} ALIGNED_TO_WORD Func_host;
DEF_SIZE(Func_host);


static Obj new_func_host(Obj sym, Int len_pars, Func_host_ptr ptr) {
  assert(obj_is_sym(sym));
  Obj o = ref_alloc(st_Func_host, size_RC + size_Func_host);
  Func_host* f = ref_body(o);
  f->sym = sym;
  f->len_pars = len_pars;
  f->ptr = ptr;
  return o;
}
