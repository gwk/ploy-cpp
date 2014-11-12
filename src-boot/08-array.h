// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// fixed-length object array type.

#include "07-obj.h"


// iterate over array using pointer Obj pointer e, start index m, end index n.
// note: c syntax requires that all declarations in the for initializer have the same type,
// or pointer of that type.
// this prevents us from declaring a tempory variable to hold the value of array,
// and so we cannot help but evaluate array multiple times.
#define it_array_from_to(it, array, from, to) \
for (Obj *it = (array).els + (from), \
*_end_##it = (array).els + (to); \
it < _end_##it; \
it++)

#define it_array_to(it, array, to) it_array_from_to(it, array, 0, to)
#define it_array_from(it, array, from) it_array_from_to(it, array, from, (array).len)
#define it_array(it, array) it_array_from(it, array, 0)


struct Array {
  Int len;
  Obj* els; // TODO: union with Obj el to optimize the len == 1 case?

  Array(): len(0), els(null) {}

  Array(Int l, Obj* e): len(l), els(e) {}

  explicit Array(Int l): len(0), els(null) {
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

  Bool operator==(Array a) {
    return len == a.len && memcmp(els, a.els, Uns(len * size_Obj)) == 0;
  }

  Obj* end() {
    return els + len;
  }

  Obj el(Int i) {
    // return element i in array with no ownership changes.
    assert(vld() && i >= 0 && i < len);
    return els[i];
  }

  Obj el_move(Int i) {
    // move element at i out of array.
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
    // note: unlike put, the memory being overwritten may not have been previously zeroed,
    // because until now it was outside of the array range.
    // during debug it is often the malloc scribble value.
    els[i] = o;
    return i;
  }

  void grow(Int new_len) {
    // note: this function does not set len,
    // because that reflects the number of elements used, not allocation size.
    // TODO: change this to match ploy Arr and List?
    assert(len < new_len);
    els = static_cast<Obj*>(raw_realloc(els, new_len * size_Obj, ci_Array));
#if OPTION_MEM_ZERO
    if (len < new_len) {
      // zero all new, uninitialized els to catch illegal derefernces.
      memset(els + len, 0, Uns((new_len - len) * size_Obj));
    }
#endif
  }

  void rel_els(Bool dbg_clear=true) {
    it_array(it, *this) {
      it->rel();
      if (OPTION_MEM_ZERO && dbg_clear) {
        *it = obj0;
      }
    }
  }

  void dissolve_els(Bool dbg_clear=true) {
    it_array(it, *this) {
      it->dissolve();
      if (OPTION_MEM_ZERO && dbg_clear) {
        *it = obj0;
      }
    }
  }

  void dealloc(Bool dbg_cleared=true) {
    // elements must have been previously moved or cleared.
    if (OPTION_MEM_ZERO && dbg_cleared) {
      it_array(it, *this) {
        assert(!it->vld());
      }
    }
    raw_dealloc(els, ci_Array);
  }

  void rel_els_dealloc() {
    rel_els(false);
    dealloc(false);
  }

  void dissolve_els_dealloc() {
    dissolve_els();
    dealloc(false);
  }

};
