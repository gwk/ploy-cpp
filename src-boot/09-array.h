// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// fixed-length object array type.

#include "08-obj.h"


class Array {
  Int _len;
  Obj* _els; // TODO: union with Obj el to optimize the len == 1 case?

public:

  Array(): _len(0), _els(null) {}

  explicit Array(Int l): _len(0), _els(null) {
    grow(l);
  }

  Array(Int l, Obj* e): _len(l), _els(e) {}

  Int len() const { return _len; }

  Obj* els() const { return _els; }
  
  Bool vld() {
    if (_els) {
      return _len >= 0;
    } else {
      return !_len;
    }
  }

  Bool operator==(Array a) {
    return _len == a._len && memcmp(_els, a._els, Uns(_len * size_Obj)) == 0;
  }

  Obj* begin() const { return _els; }

  Obj* end() const { return _els + _len; }

  Range<Obj*>to(Int to_index) const { return Range<Obj*>(begin(), begin() + to_index); }

  Obj el(Int i) {
    // return element i in array with no ownership changes.
    assert(vld() && i >= 0 && i < _len);
    return _els[i];
  }

  Obj el_move(Int i) {
    // move element at i out of array.
    Obj e = el(i);
#if OPTION_MEM_ZERO
    _els[i] = obj0;
#endif
    return e;
  }

  void put(Int i, Obj o) {
    assert(vld() && i >= 0 && i < _len);
#if OPTION_MEM_ZERO
    assert(!_els[i].vld());
#endif
    _els[i] = o;
  }

  void grow(Int new_len) {
    assert(_len < new_len);
    _els = static_cast<Obj*>(raw_realloc(_els, new_len * size_Obj, ci_Array));
#if OPTION_MEM_ZERO
    if (_len < new_len) {
      // zero all new, uninitialized els to catch illegal dereferences.
      memset(_els + _len, 0, Uns((new_len - _len) * size_Obj));
    }
#endif
    _len = new_len;
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
    raw_dealloc(_els, ci_Array);
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


static Obj Cmpd_from_Array(Obj type, Array a) {
  // owns type, elements of a.
  Obj c = Obj::Cmpd_raw(type, a.len());
  for_in(i, a.len()) {
    c.cmpd_put(i, a.el_move(i));
  }
  return c;
}

