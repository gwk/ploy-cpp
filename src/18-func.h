// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "17-file.h"


typedef struct {
  Obj sym;
  Ptr ptr;
} ALIGNED_TO_WORD Func_host;


typedef Obj(*Func_host_ptr_1)(Obj);
typedef Obj(*Func_host_ptr_2)(Obj, Obj);
typedef Obj(*Func_host_ptr_3)(Obj, Obj, Obj);


static Obj new_func_host(Tag tag, Obj sym, Ptr ptr) {
  assert(obj_is_sym(sym));
  Obj o = ref_alloc(tag, size_RC + sizeof(Func_host));
  Func_host* f = ref_body(o);
  f->sym = sym;
  f->ptr = ptr;
  return o;
}
