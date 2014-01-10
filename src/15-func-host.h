// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "14-file.h"


typedef struct {
  Ptr ptr;
  Int arg_count;
} ALIGNED_TO_WORD Func_host;


typedef Obj(*Func_host1)(Obj);
typedef Obj(*Func_host2)(Obj, Obj);
typedef Obj(*Func_host3)(Obj, Obj, Obj);


static Obj new_func_host(Ptr ptr, Int arg_count) {
  Obj o = ref_alloc(st_Func_host, size_RCL + sizeof(Func_host));
  Func_host* f = cast(Func_host*, o.rcl + 1);
  f->ptr = ptr;
  f->arg_count = arg_count;
  return ref_add_tag(o, ot_strong);
}


static Obj func_host_call(Obj func, Obj args) {
  assert(ref_is_func_host(func));
  assert(ref_is_vec(args));
  Func_host* f = cast(Func_host*, func.rcl + 1);
  Int len = ref_len(args);
  check(len == f->arg_count, "host function expects %ld arguments; received %ld",
    len, f->arg_count);
  Obj* els = ref_vec_els(args);
  #define CALL(c, ...) case c: return (cast(Func_host##c, f->ptr))(__VA_ARGS__)
  switch (f->arg_count) {
    CALL(1, obj_borrow(els[0]));
    CALL(2, obj_borrow(els[0]), obj_borrow(els[1]));
    CALL(3, obj_borrow(els[0]), obj_borrow(els[1]), obj_borrow(els[2]));
    default: error("unsupported number of arguments to host function: %ld", f->arg_count);
  }
  #undef CALL
}



