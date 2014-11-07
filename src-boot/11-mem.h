// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// fixed-length object array type.

#include "10-ref.h"


typedef struct {
  Int len;
  Obj* els; // TODO: union with Obj el to optimize the len == 1 case?
} Mem;


#define mem0 (Mem){.len=0, .els=NULL}


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


static Mem mem_mk(Int len, Obj* els) {
  return (Mem){.len=len, .els=els};
}


UNUSED_FN static Bool mem_eq(Mem a, Mem b) {
  return a.len == b.len && memcmp(a.els, b.els, cast(Uns, a.len * size_Obj)) == 0;
}


static void assert_mem_is_valid(Mem m) {
  if (m.els) {
    assert(m.len >= 0);
  } else {
    assert(m.len == 0);
  }
}


static void assert_mem_index_is_valid(Mem m, Int i) {
  assert_mem_is_valid(m);
  assert(i >= 0 && i < m.len);
}


static Mem mem_next(Mem m) {
  // note: this may produce an invalid mem representing the end of the region;
  // as a minor optimization, we do not set m.els to NULL if len == 0,
  // but we could if it matters.
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
  assert_mem_index_is_valid(m, i);
#if OPTION_MEM_ZERO
  assert(is(m.els[i], obj0));
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


UNUSED_FN static Mem mem_push(Mem m, Obj o) {
  mem_put(m, 0, o);
  return mem_next(m);
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
    assert(is(*it, obj0));
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

