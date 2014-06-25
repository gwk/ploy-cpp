// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "16-data.h"


static Obj new_vec_raw(Int len) {
  if (!len) return rc_ret_val(s_VEC0);
  Obj v = ref_alloc(rt_Vec, size_RHL + (size_Obj * len));
  v.rhl->len = len;
  return v;
}


static Obj new_vec_M(Mem m) {
  // owns elements of m.
  if (!m.len) return rc_ret_val(s_VEC0);
  Obj v = new_vec_raw(m.len);
  Obj* els = vec_ref_els(v);
  for_in(i, m.len) {
    els[i] = mem_el_move(m, i);
  }
  return v;
}


static Obj new_vec_EM(Obj el, Mem m) {
  // owns el, elements of m.
  Int len = m.len + 1;
  Obj v = new_vec_raw(len);
  Obj* els = vec_ref_els(v);
  els[0] = el;
  for_in(i, m.len) {
    els[i + 1] = mem_el_move(m, i);
  }
  return v;
}


static Obj new_vec2(Obj a, Obj b) {
  // owns all arguments.
  Obj v = new_vec_raw(2);
  Obj* els = vec_ref_els(v);
  els[0] = a;
  els[1] = b;
  return v;
}


static Obj new_vec3(Obj a, Obj b, Obj c) {
  // owns all arguments.
  Obj v = new_vec_raw(3);
  Obj* els = vec_ref_els(v);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  return v;
}


static Obj new_vec4(Obj a, Obj b, Obj c, Obj d) {
  // owns all arguments.
  Obj v = new_vec_raw(4);
  Obj* els = vec_ref_els(v);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  els[3] = d;
  return v;
}


static Int vec_ref_len(Obj v) {
  assert(ref_is_vec(v));
  assert(v.rhl->len > 0);
  return v.rhl->len;
}


static Int vec_len(Obj v) {
  if (v.u == s_VEC0.u) return 0;
  return vec_ref_len(v);
}


static Obj* vec_ref_els(Obj v) {
  assert(ref_is_vec(v));
  return cast(Obj*, v.rhl + 1); // address past rhl.
}


static Obj* vec_els(Obj v) {
  if (v.u == s_VEC0.u) return NULL;
  return vec_ref_els(v);
}


static Mem vec_ref_mem(Obj v) {
  return mem_mk(vec_ref_len(v), vec_ref_els(v));
}


static Mem vec_mem(Obj v) {
  return mem_mk(vec_len(v), vec_els(v));
}


static Obj vec_ref_el(Obj v, Int i) {
  // assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i < vec_ref_len(v));
  Obj* els = vec_ref_els(v);
  return els[i];
}


static void vec_ref_put(Obj v, Int i, Obj el) {
  // owns el.
  // assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i < vec_ref_len(v));
  Obj* els = vec_ref_els(v);
  // safe to release els[i] first, because even if el is els[i],
  // it is owned by this function so the release could never cause deallocation.
  rc_rel(els[i]);
  els[i] = el;
}


static Obj chain_hd(Obj v) {
  return vec_ref_el(v, 0);
}


static Obj chain_tl(Obj v) {
  // tl is defined to be the last element of a vec.
  // this allows us to create fat chains with more than one element per link.
  return vec_ref_el(v, vec_ref_len(v) - 1);
}


static Obj chain_a(Obj v) {
  // idiomatic accessor for fat chains.
  return vec_ref_el(v, 0);
}


static Obj chain_b(Obj v) {
  // idiomatic accessor for fat chains.
  return vec_ref_el(v, 1);
}


typedef enum {
  vs_vec,
  vs_chain,
  vs_chain_blocks,
  vs_quo,
  vs_qua,
  vs_unq,
  vs_label,
  vs_variad,
  vs_seq,
} Vec_shape;


static Vec_shape vec_ref_shape(Obj v) {
  assert(ref_is_vec(v));
  Int len = vec_len(v);
  Obj e0 = vec_ref_el(v, 0);
  if (len == 2) {
    if (e0.u == s_QUO.u) return vs_quo;
    if (e0.u == s_QUA.u) return vs_qua;
    if (e0.u == s_UNQ.u) return vs_unq;
  }
  if (len == 4 && obj_is_sym(vec_ref_el(v, 1))) {
    if (e0.u == s_LABEL.u) return vs_label;
    if (e0.u == s_VARIAD.u) return vs_variad;
  }
  if (e0.u == s_SEQ.u) return vs_seq;
  Vec_shape s = vs_chain;
  loop {
    if (vec_ref_len(v) != 2) s = vs_chain_blocks;
    Obj tl = chain_tl(v);
    if (tl.u == s_END.u) return s;
    if (!obj_is_vec_ref(tl)) return vs_vec;
    v = tl;
  }
}


static Bool vec_ref_contains_unquote(Obj v) {
  assert(ref_is_vec(v));
  Mem m = vec_ref_mem(v);
  if (m.els[0].u == s_UNQ.u) return true; // vec is an unquote form.
  it_mem_from(it, m, 1) {
    Obj e = *it;
    if (obj_is_vec_ref(e) && vec_ref_contains_unquote(e)) return true;
  }
  return false;
}

