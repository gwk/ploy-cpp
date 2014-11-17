// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// preprocessing stage.
// eliminates comment expressions from a code tree.

#include "26-exc.h"


static Obj preprocess(Obj code) {
  if (!code.is_cmpd()) {
    return code.ret();
  }
  if (code.ref_type() == t_Comment) {
    return obj0;
  }
  List dst;
  for_val(el, code.cmpd_it()) {
    Obj o = preprocess(el);
    if (o.vld()) {
      dst.append(o); // owns o.
    }
  }
  Obj c = Cmpd_from_Array(code.ref_type().ret(), dst.array());
  dst.dealloc();
  return track_src(code, c);
}
