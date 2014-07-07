// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-env.h"


struct _Vec {
  Obj type;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Vec);


static Obj vec_new_raw(Int len) {
  Obj v = ref_alloc(rt_Vec, size_Vec + (size_Obj * len));
  v.vec->type = rc_ret_val(s_Vec);
  v.vec->len = len;
  return v;
}


static Obj vec_new_M(Mem m) {
  // owns elements of m.
  Obj v = vec_new_raw(m.len);
  Obj* els = vec_els(v);
  for_in(i, m.len) {
    els[i] = mem_el_move(m, i);
  }
  return v;
}


static Obj vec_new_EM(Obj el, Mem m) {
  // owns el, elements of m.
  Int len = m.len + 1;
  Obj v = vec_new_raw(len);
  Obj* els = vec_els(v);
  els[0] = el;
  for_in(i, m.len) {
    els[i + 1] = mem_el_move(m, i);
  }
  return v;
}


static Obj vec_new1(Obj a) {
  // owns all arguments.
  Obj v = vec_new_raw(1);
  Obj* els = vec_els(v);
  els[0] = a;
  return v;
}


static Obj vec_new2(Obj a, Obj b) {
  // owns all arguments.
  Obj v = vec_new_raw(2);
  Obj* els = vec_els(v);
  els[0] = a;
  els[1] = b;
  return v;
}


static Obj vec_new3(Obj a, Obj b, Obj c) {
  // owns all arguments.
  Obj v = vec_new_raw(3);
  Obj* els = vec_els(v);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  return v;
}


static Obj vec_new4(Obj a, Obj b, Obj c, Obj d) {
  // owns all arguments.
  Obj v = vec_new_raw(4);
  Obj* els = vec_els(v);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  els[3] = d;
  return v;
}


static Int vec_len(Obj v) {
  assert(ref_is_vec(v));
  assert(v.vec->len >= 0);
  return v.vec->len;
}


static Obj* vec_els(Obj v) {
  assert(ref_is_vec(v));
  return cast(Obj*, v.vec + 1); // address past vec header.
}


static Mem vec_mem(Obj v) {
  return mem_mk(vec_len(v), vec_els(v));
}


static Obj vec_el(Obj v, Int i) {
  // assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i >= 0 && i < vec_len(v));
  Obj* els = vec_els(v);
  return els[i];
}


UNUSED_FN static void vec_put(Obj v, Int i, Obj el) {
  // owns el.
  // assumes the caller knows the size of the vector.
  assert(ref_is_vec(v));
  assert(i >= 0 && i < vec_len(v));
  Obj* els = vec_els(v);
  // safe to release els[i] first, because even if el is els[i],
  // it is owned by this function so the release could never cause deallocation.
  rc_rel(els[i]);
  els[i] = el;
}


static Obj vec_slice(Obj v, Int f, Int t) {
  // owns v.
  Int l = vec_len(v);
  if (f < 0) f += l;
  if (t < 0) t += l;
  f = int_clamp(f, 0, l);
  t = int_clamp(t, 0, l);
  Int ls = t - f; // length of slice.
  if (ls == l) {
    return v; // no ret/rel necessary.
  }
  Obj s = vec_new_raw(ls);
  Obj* src = vec_els(v);
  Obj* dst = vec_els(s);
  for_in(i, ls) {
    dst[i] = rc_ret(src[i + f]);
  }
  rc_rel(v);
  return s;
}


static Obj vec_slice_from(Obj v, Int f) {
  // owns v.
  return vec_slice(v, f, vec_len(v));
}


static Obj vec_rel_fields(Obj v) {
  Mem m = vec_mem(v);
  if (!m.len) return obj0; // termination sentinel for rc_rel tail loop.
  Int last_i = m.len - 1;
  it_mem_to(it, m, last_i) {
    rc_rel(*it);
  }
  return m.els[last_i];
}


typedef enum {
  vs_vec,
  vs_quo,
  vs_qua,
  vs_unq,
  vs_label,
  vs_variad,
  vs_seq,
} Vec_shape;


static Vec_shape vec_shape(Obj v) {
  assert(ref_is_vec(v));
  Int len = vec_len(v);
  if (!len) return vs_vec;
  Obj e0 = vec_el(v, 0);
  if (len == 2) {
    if (e0.u == s_Quo.u) return vs_quo;
    if (e0.u == s_Qua.u) return vs_qua;
    if (e0.u == s_Unq.u) return vs_unq;
  }
  if (len == 4 && obj_is_sym(vec_el(v, 1))) {
    if (e0.u == s_Label.u) return vs_label;
    if (e0.u == s_Variad.u) return vs_variad;
  }
  if (e0.u == s_Syn_seq.u) return vs_seq;
  return vs_vec;
}


static Bool vec_contains_unquote(Obj v) {
  assert(ref_is_vec(v));
  Mem m = vec_mem(v);
  if (!m.len) return false;
  if (m.els[0].u == s_Unq.u) return true; // vec is an unquote form.
  it_mem_from(it, m, 1) {
    Obj e = *it;
    if (obj_is_vec(e) && vec_contains_unquote(e)) return true;
  }
  return false;
}

