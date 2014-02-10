// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// fixed-length object array type.

#include "07-ref.h"


typedef struct {
  Int len;
  Obj* els;
} Mem;


#define mem0 (Mem){.len=0, .els=NULL}


// iterate over mem using pointer Obj pointer e, start index m, end index n.
// note: c syntax requires that all declarations in the for initializer have the same type,
// or pointer of that type.
// this prevents us from declaring a tempory variable to hold the value of mem,
// and so we cannot help but evaluate mem multiple times.
#define it_mem_from_to(it, mem, from, to) \
for (Obj *it = (mem).els + (from), \
*_##it##_end = (mem).els + (to); \
it < _##it##_end; \
it++)

#define it_mem_to(it, mem, to) it_mem_from_to(it, mem, 0, to)
#define it_mem_from(it, mem, from) it_mem_from_to(it, mem, from, (mem).len)
#define it_mem(it, mem) it_mem_from(it, mem, 0)


static Mem mem_mk(Int len, Obj* els) {
  return (Mem){.len=len, .els=els};
}


UNUSED_FN static Bool mem_eq(Mem a, Mem b) {
  return a.len == b.len && memcmp(a.els, b.els, cast(Uns, a.len * size_Obj)) == 0;
}


static void assert_mem_is_valid(Mem m) {
  assert(m.len == 0 || (m.len > 0 && m.els));
}


static void assert_mem_index_is_valid(Mem m, Int i) {
  assert_mem_is_valid(m);
  assert(i >= 0 && i < m.len);
}


static Mem mem_next(Mem m) {
  assert(m.len > 0 && m.els);
  return mem_mk(m.len - 1, m.els + 1);
}


UNUSED_FN static Obj* mem_end(Mem m) {
  return m.els + m.len;
}


static Obj mem_el(Mem m, Int i) {
  // return element i in m with no ownership changes.
  assert_mem_index_is_valid(m, i);
  return m.els[i];
}


UNUSED_FN static Obj mem_el_ret(Mem m, Int i) {
  // retain and return element i in m.
  return obj_ret(mem_el(m, i));
}


static Obj mem_el_move(Mem m, Int i) {
  // move element at i out of m.
  Obj e = mem_el(m, i);
#if OPT_ALLOC_SCRIBBLE
  m.els[i] = obj0;
#endif
  return e;
}


static Int mem_append_move(Mem* m, Obj o) {
  // move object o into m.
  // owns o.
  Int i = m->len++;
  m->els[i] = o;
  return i;
}


static void mem_realloc(Mem* m, Int len) {
  // release any truncated elements, realloc memory, and zero any new elements.
  // note that this function does not set m->len,
  // because that reflects the number of elements used, not allocation size.
  it_mem_from(it, *m, len) { // release any old elements.
    obj_rel(*it);
  }
  m->els = raw_realloc(m->els, len * size_Obj, ci_Mem);
#if OPT_ALLOC_SCRIBBLE
  if (m->len < len) {
    // zero all new, uninitialized els to catch illegal derefernces.
    memset(m->els + m->len, 0, (len - m->len) * size_Obj);
  }
#endif
}


static void mem_dealloc(Mem m) {
  // dealloc m but do not release the elements, which must have been previously moved.
#if OPT_ALLOC_SCRIBBLE
  it_mem(it, m) {
    assert(it->u == obj0.u);
  }
#endif
  raw_dealloc(m.els, ci_Mem);
}


static void mem_release_dealloc(Mem m) {
  // release all elements and dealloc m.
  it_mem(it, m) {
    obj_rel(*it);
#if OPT_ALLOC_SCRIBBLE
    *it = obj0;
#endif
  }
  mem_dealloc(m);
}

