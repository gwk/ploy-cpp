// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// resizable Array type.

#include "08-mem.h"


typedef struct {
  Mem mem;
  Int cap;
} Array;

#define array0 (Array){.mem=mem0, .cap=0}


static void assert_array_is_valid(Array* a) {
  assert(mem_is_valid(a->mem) && a->cap >= 0 && a->mem.len <= a->cap);
}


static void array_grow_cap(Array* a) {
  assert_array_is_valid(a);
  a->cap = round_up_to_power_2(a->cap + 3); // minimum capacity of 4 elements.
  mem_realloc(&a->mem, a->cap);
}


static Int array_append_move(Array* a, Obj o) {
  assert_array_is_valid(a);
  if (a->mem.len == a->cap) {
    array_grow_cap(a);
  }
  return mem_append_move(&a->mem, o);
}

