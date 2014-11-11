// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// resizable Array type.

#include "11-mem.h"


struct Array {
  Mem mem;
  Int cap;
  Array(Mem m, Int c): mem(m), cap(c) {}

  Bool vld() {
    return mem.vld() && cap >= 0 && mem.len <= cap;
  }
};

#define array0 Array(mem0, 0)


static Array array_alloc_cap(Int cap) {
  Array a = array0;
  a.cap = cap;
  a.mem.grow(a.cap);
  return a;
}


static void array_grow_cap(Array* a) {
  assert(a->vld());
  if (a->cap == 0) {
    a->cap = 2; // minimum capacity for 8 byte words with 16 byte min malloc.
  } else {
    a->cap *= 2;
  }
  a->mem.grow(a->cap);
}


static Int array_append(Array* a, Obj o) {
  // semantics can be move (owns o) or borrow (must be cleared prior to dealloc).
  assert(a->vld());
  if (a->mem.len == a->cap) {
    array_grow_cap(a);
  }
  return a->mem.append(o);
}


static Bool array_contains(Array* a, Obj r) {
  assert(a->vld());
  it_mem(it, a->mem) {
    if (it->u == r.u) {
      return true;
    }
  }
  return false;
}
