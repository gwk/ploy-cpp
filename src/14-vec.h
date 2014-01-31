// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "13-data.h"


static Obj new_vec_raw(Int len) {
  if (!len) return obj_ret_val(VEC0);
  Obj v = ref_alloc(st_Vec, size_RCL + (size_Obj * len));
  v.rcl->len = len;
  return v;
}


static Obj new_vec_M(Mem m) {
  // owns elements of m.
  if (!m.len) return obj_ret_val(VEC0);
  Obj v = new_vec_raw(m.len);
  Obj* els = vec_els(v);
  for_in(i, m.len) {
    els[i] = mem_el_move(m, i);
  }
  return v;
}


static Obj new_vec_HM(Obj hd, Mem m) {
  // owns hd, elements of m.
  Int len = m.len + 1;
  Obj v = new_vec_raw(len);
  Obj* els = vec_els(v);
  els[0] = hd;
  for_in(i, m.len) {
    els[i + 1] = mem_el_move(m, i);
  }
  return v;
}


static Obj new_vec2(Obj a, Obj b) {
  // owns all arguments.
  Obj v = new_vec_raw(2);
  Obj* els = vec_els(v);
  els[0] = a;
  els[1] = b;
  return v;
}


static Obj new_vec3(Obj a, Obj b, Obj c) {
  // owns all arguments.
  Obj v = new_vec_raw(3);
  Obj* els = vec_els(v);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  return v;
}


static Obj new_vec4(Obj a, Obj b, Obj c, Obj d) {
  // owns all arguments.
  Obj v = new_vec_raw(4);
  Obj* els = vec_els(v);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  els[3] = d;
  return v;
}



static Obj new_chain_M(Mem m) {
  // owns all elements from m.
  if (!m.len) {
    return obj_ret_val(CHAIN0);
  }
  Obj c = obj_ret_val(END);
  for_in_rev(i, m.len) {
    Obj el = mem_el_move(m, i);
    c = new_vec2(c, el); // note: unlike lisp, the tail is in position 0.
  }
  //obj_errL(c);
  return c;
}


static Obj vec_el(Obj v, Int i);
static void vec_put(Obj v, Int i, Obj el);

static Obj new_chain_blocks_M(Mem m) {
  // owns all elements from m.
  if (!m.len) {
    return obj_ret_val(CHAIN0);
  }
  Obj c = obj_ret_val(END);
  for_in_rev(i, m.len) {
    Obj el = mem_el_move(m, i);
    assert(obj_is_vec(el));
    assert(vec_el(el, 0).u == ILLEGAL.u);
    vec_put(el, 0, c);
    c = el;
  }
  return c;
}


static Obj* vec_els(Obj v) {
  assert(ref_is_vec(v));
  return cast(Obj*, v.rcl + 1); // address past rcl.
}


static Obj vec_el(Obj v, Int i) {
  // borrows el.
  // assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i < ref_len(v));
  Obj* els = vec_els(v);
  return els[i];
}


static void vec_put(Obj v, Int i, Obj el) {
  // owns el.
  // assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i < ref_len(v));
  Obj* els = vec_els(v);
  // note: the order is release, then retain. this should always be sound.
  obj_rel(els[i]);
  els[i] = obj_ret(el);
}


static Obj chain_hd(Obj v) {
  return vec_el(v, 1); // note: unlike lisp, hd is in position 1.
}


static Obj chain_tl(Obj v) {
  return vec_el(v, 0); // note: unlike lisp, tl is in position 0.
}


static Obj vec_a(Obj v) {
return vec_el(v, 1);
}


static Obj vec_b(Obj v) {
  return vec_el(v, 2);
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
    Obj tl = chain_tl(v);
    if (tl.u == END.u) return s;
    if (!obj_is_vec(tl)) return vs_vec;
    if (ref_len(v) != 2) s = vs_chain_blocks;
    v = tl;
  }
}

