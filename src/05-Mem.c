// Copyright 2011 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "04-Obj.c"


#define MEM_CLEAR_ELS 1


typedef struct {
  Obj* els;
  Int len;
} Mem;


#define mem0 (Mem){.els=NULL, .len=0}


static Mem mem_mk(Obj* els, Int len) {
  return (Mem){.els=els, .len=len};
}


static bool mem_eq(Mem a, Mem b) {
  return a.len == b.len && memcmp(a.els, b.els, (Uns)(a.len * size_Obj)) == 0;
}


static bool mem_is_valid(Mem m) {
  return (m.len == 0) || (m.els && m.len > 0);
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


static Obj mem_el(Mem m, Int i) {
  check_mem_index(m, i);
  return m.els[i]; // TODO: mark as borrowed.
}


static Int mem_append(Mem* m, Obj o) {
  Int i = m->len++;
  m->els[i] = obj_retain_strong(o);
  return i;
}


static void mem_free(Mem m) {
  for_in(i, m.len) {
    obj_release(m.els[i]);
  }
  free(m.els);
}


// release any truncated elements, realloc memory, and zero any new elements.
// does not set m.len; use mem_resize or array_grow_cap.
static void mem_realloc(Mem* m, Int len) {
  Int old_len = m->len;
  // release any old elements.
  for_imn(i, len, old_len) {
    obj_release(m->els[i]);
  }
  // realloc.
  if (len > 0) {
    m->els = realloc(m->els, (Uns)(len * size_Obj));
    check(m->els, "realloc failed; len: %ld; width: %ld", len, size_Obj);
  }
  else if (len == 0) {
    free(m->els);
    m->els = NULL;
  }
  else error("bad len: %ld", len);
  // clear any new elements.
  if (MEM_CLEAR_ELS && old_len < len) {
    memset(m->els + old_len, 0, (len - old_len) * size_Obj);
  }
}


// realloc and set size.
static void mem_resize(Mem* m, Int len) {
  mem_realloc(m, len);
  m->len = len;
}


