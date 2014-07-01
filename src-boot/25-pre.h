// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// preprocessing stage.
// eliminates comment expressions from a code tree.

#include "24-exc.h"


static Obj preprocess(Obj code) {
  if (!obj_is_vec_ref(code)) {
    return rc_ret(code);
  }
  Mem src = vec_ref_mem(code);
  if (src.els[0].u == s_COMMENT.u) {
    return obj0;
  }
  Array dst = array0;
  it_mem(it, src) {
    Obj o = preprocess(*it);
    if (o.u != obj0.u) {
      array_append(&dst, o); // owns o.
    }
  }
  Obj v = vec_new_M(dst.mem);
  mem_dealloc(dst.mem);
  return v;
}
