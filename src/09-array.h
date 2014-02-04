// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "08-mem.h"


typedef struct {
  Mem mem;
  Int cap;
} Array;

#define array0 (Array){.mem=mem0, .cap=0}


static bool array_is_valid(Array a) {
  return mem_is_valid(a.mem) && a.cap >= 0 && a.mem.len <= a.cap;
}


static Array array_with_len(Int len) {
  Array a = array0;
  a.cap = round_up_to_power_2(len);
  mem_realloc(&a.mem, a.cap);
  a.mem.len = len;
  return a;
}


static void array_grow_cap(Array* a) {
  a->cap = round_up_to_power_2(a->cap + 3); // minimum capacity of 4 elements.
  mem_realloc(&a->mem, a->cap);
}


static Int array_append_move(Array* a, Obj o) {
  if (a->mem.len == a->cap) {
    array_grow_cap(a);
  }
  return mem_append_move(&a->mem, o);
}

