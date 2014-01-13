// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "12-data.h"


static Obj new_vec_M(Mem m) {
  if (!m.len) {
    return VEC0;
  }
  Obj v = ref_alloc(st_Vec, size_RCL + size_Obj * m.len);
  v.rcl->len = m.len;
  Obj* p = ref_vec_els(v);
  for_in(i, m.len) {
    Obj el = mem_el(m, i);
    assert_obj_is_strong(el);
    p[i] = el;
  }
  return ref_add_tag(v, ot_strong);
}


static Obj new_vec2(Obj a, Obj b) {
  Mem m = mem_mk((Obj[]){a, b}, 2);
  return new_vec_M(m);
}


static Obj new_vec3(Obj a, Obj b, Obj c) {
  Mem m = mem_mk((Obj[]){a, b, c}, 3);
  return new_vec_M(m);
}


static Obj new_chain_M(Mem m) {
  if (!m.len) {
    return CHAIN0;
  }
  Obj c = END;
  for_in_rev(i, m.len) {
    Obj el = mem_el(m, i);
    c = new_vec2(el, c);
  }
  //obj_errL(c);
  return c;
}


static Obj new_chain_2(Obj hd, Obj tl) {
  if (tl.u == CHAIN0.u) {
    tl = CHAIN0;
  }
  return new_vec2(hd, tl);
}


static Obj vec_el(Obj v, Int i) {
  // this assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i < ref_len(v));
  Obj* els = ref_vec_els(v);
  return obj_borrow(els[i]);
}


static void vec_put(Obj v, Int i, Obj o) {
  // this assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i < ref_len(v));
  Obj* els = ref_vec_els(v);
  els[i] = obj_retain_strong(o);
}


static Obj vec_hd(Obj v) {
  return vec_el(v, 0);
}


static Obj vec_tl(Obj v) {
  return vec_el(v, 1);
}


static Obj vec_a(Obj v) {
return vec_el(v, 0);
}


static Obj vec_b(Obj v) {
  return vec_el(v, 1);
}


typedef enum {
  vs_vec,
  vs_chain,
  vs_chain_blocks,
} Vec_shape;


static Vec_shape vec_shape(Obj v) {
  assert(ref_is_vec(v));
  Vec_shape s = vs_chain;
  loop {
    Obj last = vec_tl(v);
    if (last.u == END.u) return s;
    if (!obj_is_ref(last)) return vs_vec;
    Obj r = obj_ref_borrow(last);
    if (!ref_is_vec(r)) return vs_vec;
    if (ref_len(v) != 2) s = vs_chain_blocks;
    v = r;
  }
}

