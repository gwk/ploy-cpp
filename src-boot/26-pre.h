// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// preprocessing stage.
// eliminates comment expressions from a code tree.

#include "25-exc.h"


static Obj preprocess(Obj code) {
  if (!obj_is_cmpd(code)) {
    return rc_ret(code);
  }
  Mem src = cmpd_mem(code);
  if (is(ref_type(code), t_Comment)) {
    return obj0;
  }
  Array dst = array0;
  it_mem(it, src) {
    Obj o = preprocess(*it);
    if (!is(o, obj0)) {
      array_append(&dst, o); // owns o.
    }
  }
  Obj c = cmpd_new_M(rc_ret(ref_type(code)), dst.mem);
  mem_dealloc(dst.mem);
  return c;
}
