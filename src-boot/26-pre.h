// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// preprocessing stage.
// eliminates comment expressions from a code tree.

#include "25-exc.h"


static Obj preprocess(Obj code) {
  if (!code.is_cmpd()) {
    return code.ret();
  }
  if (code.ref_type() == t_Comment) {
    return obj0;
  }
  List dst;
  it_array(it, cmpd_array(code)) {
    Obj o = preprocess(*it);
    if (o.vld()) {
      dst.append(o); // owns o.
    }
  }
  Obj c = cmpd_new_M(code.ref_type().ret(), dst.array);
  dst.array.dealloc();
  return track_src(code, c);
}
