// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// fixed-length object array type.

#include "10-ref.h"


struct Mem {
  Int len;
  Obj* els; // TODO: union with Obj el to optimize the len == 1 case?
  Mem(Int l, Obj* e): len(l), els(e) {}

  Bool vld() {
    if (els) {
      return len >= 0;
    } else {
      return !len;
    }
  }
};


#define mem0 Mem(0, null)


// iterate over mem using pointer Obj pointer e, start index m, end index n.
// note: c syntax requires that all declarations in the for initializer have the same type,
// or pointer of that type.
// this prevents us from declaring a tempory variable to hold the value of mem,
// and so we cannot help but evaluate mem multiple times.
#define it_mem_from_to(it, mem, from, to) \
for (Obj *it = (mem).els + (from), \
*_end_##it = (mem).els + (to); \
it < _end_##it; \
it++)

#define it_mem_to(it, mem, to) it_mem_from_to(it, mem, 0, to)
#define it_mem_from(it, mem, from) it_mem_from_to(it, mem, from, (mem).len)
#define it_mem(it, mem) it_mem_from(it, mem, 0)


UNUSED_FN static Bool mem_eq(Mem a, Mem b) {
  return a.len == b.len && memcmp(a.els, b.els, cast(Uns, a.len * size_Obj)) == 0;
}


UNUSED_FN static Obj* mem_end(Mem m) {
  return m.els + m.len;
}


static Obj mem_el(Mem m, Int i) {
  // return element i in m with no ownership changes.
  assert(m.vld() && i >= 0 && i < m.len);
  return m.els[i];
}


static Obj mem_el_ret(Mem m, Int i) {
  // retain and return element i in m.
  return rc_ret(mem_el(m, i));
}


static Obj mem_el_move(Mem m, Int i) {
  // move element at i out of m.
  Obj e = mem_el(m, i);
#if OPTION_MEM_ZERO
  m.els[i] = obj0;
#endif
  return e;
}


static void mem_put(Mem m, Int i, Obj o) {
  assert(m.vld() && i >= 0 && i < m.len);
#if OPTION_MEM_ZERO
  assert(!m.els[i].vld());
#endif
  m.els[i] = o;
}


static Int mem_append(Mem* m, Obj o) {
  // semantics can be move (owns o) or borrow (must be cleared prior to dealloc).
  Int i = m->len++;
  // note: unlike mem_put, the memory being overwritten may not have been previously zeroed,
  // because until now it was outside of the mem range.
  // during debug it is often the malloc scribble value.
  m->els[i] = o;
  return i;
}


static void mem_realloc(Mem* m, Int len) {
  // realloc memory and zero any new elements.
  // note: this function does not set m->len,
  // because that reflects the number of elements used, not allocation size.
  assert(m->len < len);
  m->els = cast(Obj*, raw_realloc(m->els, len * size_Obj, ci_Mem));
#if OPTION_MEM_ZERO
  if (m->len < len) {
    // zero all new, uninitialized els to catch illegal derefernces.
    memset(m->els + m->len, 0, cast(Uns, (len - m->len) * size_Obj));
  }
#endif
}


static Mem mem_alloc(Int len) {
  Mem m = mem0;
  mem_realloc(&m, len);
  m.len = len;
  return m;
}


static void mem_rel_no_clear(Mem m) {
  // release all elements without clearing them; useful for debugging final teardown.
  it_mem(it, m) {
    rc_rel(*it);
  }
}


static void mem_rel(Mem m) {
  it_mem(it, m) {
    rc_rel(*it);
#if OPTION_MEM_ZERO
    *it = obj0;
#endif
  }
}


#if OPTION_ALLOC_COUNT
static void mem_dissolve(Mem m) {
  it_mem(it, m) {
    rc_dissolve(*it);
  }
}
#endif


static void mem_dealloc_no_clear(Mem m) {
  raw_dealloc(m.els, ci_Mem);
}


static void mem_dealloc(Mem m) {
  // elements must have been previously moved or cleared.
#if OPTION_MEM_ZERO
  it_mem(it, m) {
    assert(!it->vld());
  }
#endif
  mem_dealloc_no_clear(m);
}



static void mem_rel_dealloc(Mem m) {
  mem_rel_no_clear(m);
  mem_dealloc_no_clear(m);
}


#if OPTION_ALLOC_COUNT
static void mem_dissolve_dealloc(Mem m) {
  mem_dissolve(m);
  mem_dealloc_no_clear(m);
}
#endif

