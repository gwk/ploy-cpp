// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// preprocessing stage.
// eliminates comment expressions from a code tree.

#include "27-exc.h"


static Obj preprocess(Obj code) {
  if (!code.is_cmpd()) {
    return code.ret();
  }
  if (ref_type(code) == t_Comment) {
    return obj0;
  }
  List dst;
  it_mem(it, cmpd_mem(code)) {
    Obj o = preprocess(*it);
    if (o.vld()) {
      dst.append(o); // owns o.
    }
  }
  Obj c = cmpd_new_M(ref_type(code).ret(), dst.mem);
  dst.mem.dealloc();
  return track_src(code, c);
}
