// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "06-ref.h"


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


static Obj mem_el_borrowed(Mem m, Int i) {
  check_mem_index(m, i);
  return m.els[i];
}


static Obj mem_el_ret(Mem m, Int i) {
  return obj_ret(mem_el_borrowed(m, i));
}


static const Obj VOID;

static Obj mem_el_move(Mem m, Int i) {
  Obj e = mem_el_borrowed(m, i);
#if DEBUG
  m.els[i] = VOID;
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
#if OPT_ALLOC_COUNT
  if (m.els) total_deallocs_mem++;
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
  // realloc.
  if (len > 0) {
#if OPT_ALLOC_COUNT
    if (!m->els) total_allocs_mem++;
#endif
    m->els = realloc(m->els, (Uns)(len * size_Obj));
    check(m->els, "realloc failed; len: %ld; width: %ld", len, size_Obj);
  }
  else if (len == 0) {
    mem_dealloc(*m); // realloc does something different, plus we must count deallocs.
    m->els = NULL;
  }
  else error("bad len: %ld", len);
  // clear any new elements.
  if (OPT_MEM_CLEAR_ELS && old_len < len) {
    memset(m->els + old_len, 0, (len - old_len) * size_Obj);
  }
}


static void mem_resize(Mem* m, Int len) {
  // realloc and set size.
  mem_realloc(m, len);
  m->len = len;
}

