// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "19-file.h"


typedef Obj(*Func_host_ptr)(Obj, Mem);


struct _Func_host {
  Obj type;
  Obj sym;
  Int len_pars;
  Func_host_ptr ptr;
} ALIGNED_TO_WORD;
DEF_SIZE(Func_host);


static Obj new_func_host(Obj sym, Int len_pars, Func_host_ptr ptr) {
  assert(obj_is_sym(sym));
  Obj o = ref_alloc(rt_Func_host, size_Func_host);
  o.func_host->type = s_FUNC_HOST;
  o.func_host->sym = sym;
  o.func_host->len_pars = len_pars;
  o.func_host->ptr = ptr;
  return o;
}
