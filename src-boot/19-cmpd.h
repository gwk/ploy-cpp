// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-env.h"


// iterate over a compound.
#define it_cmpd(it, c) \
for (Obj *it = c.cmpd_els(), *_end_##it = it + c.cmpd_len(); \
it < _end_##it; \
it++)


static void cmpd_put(Obj c, Int i, Obj el);

static Obj cmpd_new_raw(Obj type, Int len) {
  // owns type.
  counter_inc(ci_Cmpd_rc);
  Obj o = Obj(raw_alloc(size_Cmpd + (size_Obj * len), ci_Cmpd_alloc));
  *o.h = Head(type.r);
  o.c->len = len;
#if OPTION_MEM_ZERO
  memset(o.cmpd_els(), 0, Uns(size_Obj * len));
#endif
  return o;
}


static Obj cmpd_new_M(Obj type, Array a) {
  // owns type, elements of a.
  Obj c = cmpd_new_raw(type, a.len());
  for_in(i, a.len()) {
    cmpd_put(c, i, a.el_move(i));
  }
  return c;
}


static Obj _cmpd_new(Obj type, Int i, Obj el) {
  // owns all arguments.
  Obj o = cmpd_new_raw(type, i + 1);
  cmpd_put(o, i, el);
  return o;
}

template <typename T, typename... Ts>
static Obj _cmpd_new(Obj type, Int i, T el, Ts... rest) {
  // owns all arguments.
  Obj o = _cmpd_new(type, i + 1, rest...);
  cmpd_put(o, i, el);
  return o;
}

template <typename T, typename... Ts>
static Obj cmpd_new(Obj type, T el, Ts... rest) {
  // owns all arguments.
  return _cmpd_new(type, 0, el, rest...);
}


static Array cmpd_array(Obj c) {
  return Array(c.cmpd_len(), c.cmpd_els());
}


static Obj cmpd_el(Obj c, Int i) {
  // assumes the caller knows the size of the compound.
  assert(c.ref_is_cmpd());
  return cmpd_array(c).el(i);
}


static void cmpd_put(Obj c, Int i, Obj e) {
  cmpd_array(c).put(i, e);
}


static Obj cmpd_rel_fields(Obj c) {
  Array a = cmpd_array(c);
  if (!a.len()) return obj0; // return the termination sentinel for rc_rel tail loop.
  Int last_i = a.len() - 1;
  for_mut(el, a.to(last_i)) {
    el.rel();
  }
#if OPTION_TCO
  return a.el(last_i);
#else
  a.el(last_i).rel();
  return obj0;
#endif
}


static void cmpd_dissolve_fields(Obj c) {
  it_cmpd(it, c) {
    it->rel();
    *it = s_DISSOLVED.ret_val();
  }
}

