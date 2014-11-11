// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// fixed-length object array type.

#include "10-ref.h"


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


struct Mem {
  Int len;
  Obj* els; // TODO: union with Obj el to optimize the len == 1 case?

  Mem(): len(0), els(null) {}

  Mem(Int l, Obj* e): len(l), els(e) {}

  explicit Mem(Int l): len(0), els(null) {
    grow(l);
    len = l;
  }

  Bool vld() {
    if (els) {
      return len >= 0;
    } else {
      return !len;
    }
  }

  UNUSED_FN Bool operator==(Mem m) {
    return len == m.len && memcmp(els, m.els, cast(Uns, len * size_Obj)) == 0;
  }

  UNUSED_FN Obj* end() {
    return els + len;
  }

  Obj el(Int i) {
    // return element i in m with no ownership changes.
    assert(vld() && i >= 0 && i < len);
    return els[i];
  }

  Obj el_move(Int i) {
    // move element at i out of m.
    Obj e = el(i);
#if OPTION_MEM_ZERO
    els[i] = obj0;
#endif
    return e;
  }

  void put(Int i, Obj o) {
    assert(vld() && i >= 0 && i < len);
#if OPTION_MEM_ZERO
    assert(!els[i].vld());
#endif
    els[i] = o;
  }

  Int append(Obj o) {
    // semantics can be move (owns o) or borrow (must be cleared prior to dealloc).
    Int i = len++;
    // note: unlike mem_put, the memory being overwritten may not have been previously zeroed,
    // because until now it was outside of the mem range.
    // during debug it is often the malloc scribble value.
    els[i] = o;
    return i;
  }

  void grow(Int new_len) {
    // note: this function does not set m->len,
    // because that reflects the number of elements used, not allocation size.
    // TODO: change this to match ploy Arr and List.
    assert(len < new_len);
    els = cast(Obj*, raw_realloc(els, new_len * size_Obj, ci_Mem));
#if OPTION_MEM_ZERO
    if (len < new_len) {
      // zero all new, uninitialized els to catch illegal derefernces.
      memset(els + len, 0, cast(Uns, (new_len - len) * size_Obj));
    }
#endif
  }

  void rel_no_clear() {
    // release all elements without clearing them; useful for debugging final teardown.
    it_mem(it, *this) {
      it->rel();
    }
  }

  void rel() {
    it_mem(it, *this) {
      it->rel();
#if OPTION_MEM_ZERO
      *it = obj0;
#endif
    }
  }

#if OPTION_ALLOC_COUNT
  void dissolve() {
    it_mem(it, *this) {
      it->dissolve();
    }
  }
#endif

  void dealloc_no_clear() {
    raw_dealloc(els, ci_Mem);
  }

  void dealloc() {
    // elements must have been previously moved or cleared.
#if OPTION_MEM_ZERO
    it_mem(it, *this) {
      assert(!it->vld());
    }
#endif
    dealloc_no_clear();
  }

  void rel_dealloc() {
    rel_no_clear();
    dealloc_no_clear();
  }


#if OPTION_ALLOC_COUNT
  void dissolve_dealloc() {
    dissolve();
    dealloc_no_clear();
  }
#endif

};
