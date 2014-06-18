// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// resizable Array type.

#include "10-mem.h"


typedef struct {
  Mem mem;
  Int cap;
} Array;

#define array0 (Array){.mem=mem0, .cap=0}


static void assert_array_is_valid(Array* a) {
  assert_mem_is_valid(a->mem);
  assert(a->cap >= 0 && a->mem.len <= a->cap);
}


static void array_grow_cap(Array* a) {
  assert_array_is_valid(a);
  if (a->cap == 0) {
    a->cap = 2; // minimum capacity.
  } else {
    a->cap *= 2;
  }
  mem_realloc(&a->mem, a->cap);
}


static Int array_append(Array* a, Obj o) {
  // semantics can be move (owns o) or borrow (must be cleared prior to dealloc).
  assert_array_is_valid(a);
  if (a->mem.len == a->cap) {
    array_grow_cap(a);
  }
  return mem_append(&a->mem, o);
}


static Bool array_contains(Array* a, Obj r) {
  assert_array_is_valid(a);
  it_mem(it, a->mem) {
    if (it->u == r.u) {
      return true;
    }
  }
  return false;
}
