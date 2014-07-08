// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// preprocessing stage.
// eliminates comment expressions from a code tree.

#include "23-exc.h"


static Obj preprocess(Obj code) {
  if (!obj_is_struct(code)) {
    return rc_ret(code);
  }
  Mem src = struct_mem(code);
  if (src.els[0].u == s_Comment.u) {
    return obj0;
  }
  Array dst = array0;
  it_mem(it, src) {
    Obj o = preprocess(*it);
    if (o.u != obj0.u) {
      array_append(&dst, o); // owns o.
    }
  }
  Obj s = struct_new_M(rc_ret(s_Vec_Expr), dst.mem);
  mem_dealloc(dst.mem);
  return s;
}
