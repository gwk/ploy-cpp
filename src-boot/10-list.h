// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// resizable Array type.

#include "09-array.h"


struct List {
  Array _array;
  Int len;

  List(): _array(), len(0) {}

  explicit List(Int cap): _array(), len(0) {
    _array.grow(cap);
  }

  Bool vld() {
    return _array.vld() && len >= 0 && len <= _array.len();
  }

  Obj* begin() const {
    return _array.begin();
  }

  Obj* end() const {
    return _array.begin() + len;
  }

  void grow_cap() {
    assert(vld());
    // minimum capacity is 2 given 8 byte words with 16 byte min malloc.
    Int cap = (_array.len() == 0) ? 2 : _array.len() * 2;
    _array.grow(cap);
  }

  Obj el(Int i) {
    // return element i in array with no ownership changes.
    assert(vld() && i < len);
    return _array.el(i);
  }

  Obj el_move(Int i) {
    // move element at i out of array.
    assert(vld() && i < len);
    return _array.el_move(i);
  }

  void put(Int i, Obj o) {
    assert(vld() && i < len);
    _array.put(i, o);
  }

  Int append(Obj o) {
    // semantics can be move (owns o) or borrow.
    assert(vld());
    if (len == _array.len()) {
      grow_cap();
    }
    Int i = len++;
    _array.put(i, o);
    return i;
  }

  Bool contains(Obj o) {
    assert(vld());
    for_val(el, *this) {
      if (el == o) {
        return true;
      }
    }
    return false;
  }

  Array array() {
    return Array(len, len ? _array.els() : null);
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
    _array.dealloc(dbg_cleared);
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

