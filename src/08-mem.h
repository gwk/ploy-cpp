// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "07-ref.h"


typedef struct {
  Int len;
  Obj* els;
} Mem;


#define mem0 (Mem){.len=0, .els=NULL}


static Mem mem_mk(Int len, Obj* els) {
  return (Mem){.len=len, .els=els};
}


static bool mem_eq(Mem a, Mem b) {
  return a.len == b.len && memcmp(a.els, b.els, cast(Uns, a.len * size_Obj)) == 0;
}


static bool mem_is_valid(Mem m) {
  return (m.len == 0) || (m.len > 0 && m.els);
}


static bool mem_index_is_valid(Mem m, Int i) {
  return i >= 0 && i < m.len;
}


static void check_mem_index(Mem m, Int i) {
  assert(mem_is_valid(m));
  check(mem_index_is_valid(m, i), "invalid Mem index: %ld", i);
}


static Obj* mem_end(Mem m) {
  return m.els + m.len;
}


static Obj mem_el_borrowed(Mem m, Int i) {
  check_mem_index(m, i);
  return m.els[i];
}


static Obj mem_el_ret(Mem m, Int i) {
  return obj_ret(mem_el_borrowed(m, i));
}


static const Obj ILLEGAL;

static Obj mem_el_move(Mem m, Int i) {
  Obj e = mem_el_borrowed(m, i);
#if DEBUG
  m.els[i] = ILLEGAL;
#endif
  return e;
}


static Int mem_append_move(Mem* m, Obj o) {
  Int i = m->len++;
  m->els[i] = o;
  return i;
}


static void mem_release_els(Mem m) {
  for_in(i, m.len) {
    obj_rel(m.els[i]);
  }
}


static void mem_dealloc(Mem m) {
  if (m.els) counter_dec(ci_Mem);
#if OPT_CLEAR_ELS
  memset(m.els, 0, m.len * size_Obj);
#endif
  free(m.els);
}


static void mem_release_dealloc(Mem m) {
  mem_release_els(m);
  mem_dealloc(m);
}


static void mem_realloc(Mem* m, Int len) {
  // release any truncated elements, realloc memory, and zero any new elements.
  // does not set m.len; use mem_resize or array_grow_cap.
  Int old_len = m->len;
  // release any old elements.
  for_imn(i, len, old_len) {
    obj_rel(m->els[i]);
  }
  m->els = raw_realloc(m->els, len * size_Obj, ci_Mem);
  // clear any new elements.
  if (OPT_CLEAR_ELS && old_len < len) {
    memset(m->els + old_len, 0, (len - old_len) * size_Obj);
  }
}


static void mem_resize(Mem* m, Int len) {
  // realloc and set size.
  mem_realloc(m, len);
  m->len = len;
}
