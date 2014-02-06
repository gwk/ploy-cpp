// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-data.h"


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


static Obj new_vec_EM(Obj el, Mem m) {
  // owns el, elements of m.
  Int len = m.len + 1;
  Obj v = new_vec_raw(len);
  Obj* els = vec_els(v);
  els[0] = el;
  for_in(i, m.len) {
    els[i + 1] = mem_el_move(m, i);
  }
  return v;
}


static Obj new_vec_EEM(Obj e0, Obj e1, Mem m) {
  // owns e0, e1, elements of m.
  Int len = m.len + 2;
  Obj v = new_vec_raw(len);
  Obj* els = vec_els(v);
  els[0] = e0;
  els[1] = e1;
  for_in(i, m.len) {
    els[i + 2] = mem_el_move(m, i);
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


static Int vec_len(Obj v) {
  assert(ref_is_vec(v));
  assert(v.rcl->len > 0);
  return v.rcl->len;
}


static Obj* vec_els(Obj v) {
  assert(ref_is_vec(v));
  return cast(Obj*, v.rcl + 1); // address past rcl.
}


static Obj vec_el(Obj v, Int i) {
  // assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i < vec_len(v));
  Obj* els = vec_els(v);
  return els[i];
}


static void vec_put(Obj v, Int i, Obj el) {
  // owns el.
  // assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i < vec_len(v));
  Obj* els = vec_els(v);
  obj_rel(els[i]); // safe to release els[i] first, el is owned by this function so even if it is the same as els[i] it is safe.
  els[i] = el;
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
  vs_label,
  vs_variad,
} Vec_shape;


static Vec_shape vec_shape(Obj v) {
  assert(ref_is_vec(v));
  Int len = vec_len(v);
  if (len == 4 && obj_is_symbol(vec_el(v, 1))) {
    Obj e0 = vec_el(v, 0);
    if (e0.u == LABEL.u) return vs_label;
    if (e0.u == VARIAD.u) return vs_variad;
  }
  Vec_shape s = vs_chain;
  loop {
    if (vec_len(v) != 2) s = vs_chain_blocks;
    Obj tl = chain_tl(v);
    if (tl.u == END.u) return s;
    if (!obj_is_vec_ref(tl)) return vs_vec;
    v = tl;
  }
}

