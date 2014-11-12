// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// fixed-length object array type.

#include "08-obj.h"


struct Array {
  Int len;
  Obj* els; // TODO: union with Obj el to optimize the len == 1 case?

  Array(): len(0), els(null) {}

  explicit Array(Int l): len(0), els(null) {
    grow(l);
  }

  Array(Int l, Obj* e): len(l), els(e) {}

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

  Obj* begin() const { return els; }

  Obj* end() const { return els + len; }

  Range<Obj*>to(Int to_index) const { return Range<Obj*>(begin(), begin() + to_index); }

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

  void grow(Int new_len) {
    assert(len < new_len);
    els = static_cast<Obj*>(raw_realloc(els, new_len * size_Obj, ci_Array));
#if OPTION_MEM_ZERO
    if (len < new_len) {
      // zero all new, uninitialized els to catch illegal derefernces.
      memset(els + len, 0, Uns((new_len - len) * size_Obj));
    }
#endif
    len = new_len;
  }

  void rel_els(Bool dbg_clear=true) {
    for_mut(el, *this) {
      el.rel();
      if (OPTION_MEM_ZERO && dbg_clear) {
        el = obj0;
      }
    }
  }

  void dissolve_els(Bool dbg_clear=true) {
    for_mut(el, *this) {
      el.dissolve();
      if (OPTION_MEM_ZERO && dbg_clear) {
        el = obj0;
      }
    }
  }

  void dealloc(Bool dbg_cleared=true) {
    // elements must have been previously moved or cleared.
    if (OPTION_MEM_ZERO && dbg_cleared) {
#if !OPT
      for_val(el, *this) {
        assert(!el.vld());
      }
#endif
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
