// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// resizable Array type.

#include "11-mem.h"


struct List {
  Array array;
  Int cap;

  List(): array(), cap(0) {}

  explicit List(Int c): array(), cap(c) {
    array.grow(cap);
  }

  List(Array m, Int c): array(m), cap(c) {}

  Bool vld() {
    return array.vld() && cap >= 0 && array.len <= cap;
  }

  void grow_cap() {
    assert(vld());
    if (cap == 0) {
      cap = 2; // minimum capacity for 8 byte words with 16 byte min malloc.
    } else {
      cap *= 2;
    }
    array.grow(cap);
  }

  Int append(Obj o) {
    // semantics can be move (owns o) or borrow (must be cleared prior to dealloc).
    assert(vld());
    if (array.len == cap) {
      grow_cap();
    }
    return array.append(o);
  }

  Bool contains(Obj r) {
    assert(vld());
    it_array(it, array) {
      if (it->u == r.u) {
        return true;
      }
    }
    return false;
  }

};

