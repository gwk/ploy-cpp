// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// resizable Array type.

#include "11-mem.h"


struct Array {
  Mem mem;
  Int cap;

  Array(): mem(), cap(0) {}

  explicit Array(Int c): mem(), cap(c) {
    mem.grow(cap);
  }

  Array(Mem m, Int c): mem(m), cap(c) {}

  Bool vld() {
    return mem.vld() && cap >= 0 && mem.len <= cap;
  }

  void grow_cap() {
    assert(vld());
    if (cap == 0) {
      cap = 2; // minimum capacity for 8 byte words with 16 byte min malloc.
    } else {
      cap *= 2;
    }
    mem.grow(cap);
  }

  Int append(Obj o) {
    // semantics can be move (owns o) or borrow (must be cleared prior to dealloc).
    assert(vld());
    if (mem.len == cap) {
      grow_cap();
    }
    return mem.append(o);
  }

  Bool contains(Obj r) {
    assert(vld());
    it_mem(it, mem) {
      if (it->u == r.u) {
        return true;
      }
    }
    return false;
  }

};

