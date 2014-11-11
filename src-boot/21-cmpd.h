// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "20-env.h"


// iterate over a compound.
#define it_cmpd(it, c) \
for (Obj *it = cmpd_els(c), *_end_##it = it + cmpd_len(c); \
it < _end_##it; \
it++)


struct Cmpd {
  Head head;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Cmpd);


static Obj* cmpd_els(Obj c);
static void cmpd_put(Obj c, Int i, Obj el);

static Obj cmpd_new_raw(Obj type, Int len) {
  // owns type.
  Obj c = ref_new(size_Cmpd + (size_Obj * len), type);
  c.c->len = len;
#if OPTION_MEM_ZERO
  memset(cmpd_els(c), 0, cast(Uns, (size_Obj * len)));
#endif
  return c;
}


static Obj cmpd_new_M(Obj type, Mem m) {
  // owns type, elements of m.
  Obj c = cmpd_new_raw(type, m.len);
  Obj* els = cmpd_els(c);
  for_in(i, m.len) {
    els[i] = mem_el_move(m, i);
  }
  return c;
}


UNUSED_FN static Obj cmpd_new_M_ret(Obj type, Mem m) {
  // owns type.
  Obj c = cmpd_new_raw(type, m.len);
  Obj* els = cmpd_els(c);
  for_in(i, m.len) {
    els[i] = mem_el_ret(m, i);
  }
  return c;
}


UNUSED_FN static Obj cmpd_new_EM(Obj type, Obj el, Mem m) {
  // owns type, el, elements of m.
  Int len = m.len + 1;
  Obj c = cmpd_new_raw(type, len);
  Obj* els = cmpd_els(c);
  els[0] = el;
  for_in(i, m.len) {
    els[i + 1] = mem_el_move(m, i);
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


static Int cmpd_len(Obj c) {
  assert(ref_is_cmpd(c));
  assert(c.c->len >= 0);
  return c.c->len;
}


static Obj* cmpd_els(Obj c) {
  assert(ref_is_cmpd(c));
  return cast(Obj*, c.c + 1); // address past header.
}


static Mem cmpd_mem(Obj c) {
  return Mem(cmpd_len(c), cmpd_els(c));
}


static Obj cmpd_el(Obj c, Int i) {
  // assumes the caller knows the size of the compound.
  assert(ref_is_cmpd(c));
  return mem_el(cmpd_mem(c), i);
}


static void cmpd_put(Obj c, Int i, Obj e) {
  mem_put(cmpd_mem(c), i, e);
}


static Obj cmpd_slice(Obj c, Int f, Int t) {
  assert(ref_is_cmpd(c));
  Int l = cmpd_len(c);
  if (f < 0) f += l;
  if (t < 0) t += l;
  f = int_clamp(f, 0, l);
  t = int_clamp(t, 0, l);
  Int ls = t - f; // length of slice.
  if (ls == l) {
    return c; // no ret/rel necessary.
  }
  Obj slice = cmpd_new_raw(rc_ret(ref_type(c)), ls);
  Obj* src = cmpd_els(c);
  Obj* dst = cmpd_els(slice);
  for_in(i, ls) {
    dst[i] = rc_ret(src[i + f]);
  }
  return slice;
}


static Obj cmpd_rel_fields(Obj c) {
  Mem m = cmpd_mem(c);
  if (!m.len) return obj0; // return the termination sentinel for rc_rel tail loop.
  Int last_i = m.len - 1;
  it_mem_to(it, m, last_i) {
    rc_rel(*it);
  }
#if OPTION_TCO
  return m.els[last_i];
#else
  rc_rel(m.els[last_i]);
  return obj0;
#endif
}


#if OPTION_ALLOC_COUNT
static void cmpd_dissolve_fields(Obj c) {
  it_cmpd(it, c) {
    rc_rel(*it);
    *it = rc_ret_val(s_DISSOLVED);
  }
}
#endif

