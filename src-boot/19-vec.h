// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-env.h"


struct _Struct {
  Obj type;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Struct);


static Obj struct_new_raw(Int len) {
  Obj v = ref_alloc(rt_Struct, size_Struct + (size_Obj * len));
  v.s->type = rc_ret_val(s_Struct);
  v.s->len = len;
  return v;
}


static Obj struct_new_M(Mem m) {
  // owns elements of m.
  Obj v = struct_new_raw(m.len);
  Obj* els = struct_els(v);
  for_in(i, m.len) {
    els[i] = mem_el_move(m, i);
  }
  return v;
}


static Obj struct_new_EM(Obj el, Mem m) {
  // owns el, elements of m.
  Int len = m.len + 1;
  Obj v = struct_new_raw(len);
  Obj* els = struct_els(v);
  els[0] = el;
  for_in(i, m.len) {
    els[i + 1] = mem_el_move(m, i);
  }
  return v;
}


static Obj struct_new1(Obj a) {
  // owns all arguments.
  Obj v = struct_new_raw(1);
  Obj* els = struct_els(v);
  els[0] = a;
  return v;
}


static Obj struct_new2(Obj a, Obj b) {
  // owns all arguments.
  Obj v = struct_new_raw(2);
  Obj* els = struct_els(v);
  els[0] = a;
  els[1] = b;
  return v;
}


static Obj struct_new3(Obj a, Obj b, Obj c) {
  // owns all arguments.
  Obj v = struct_new_raw(3);
  Obj* els = struct_els(v);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  return v;
}


static Obj struct_new4(Obj a, Obj b, Obj c, Obj d) {
  // owns all arguments.
  Obj v = struct_new_raw(4);
  Obj* els = struct_els(v);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  els[3] = d;
  return v;
}


static Int struct_len(Obj v) {
  assert(ref_is_struct(v));
  assert(v.s->len >= 0);
  return v.s->len;
}


static Obj* struct_els(Obj v) {
  assert(ref_is_struct(v));
  return cast(Obj*, v.s + 1); // address past struct header.
}


static Mem struct_mem(Obj v) {
  return mem_mk(struct_len(v), struct_els(v));
}


static Obj struct_el(Obj v, Int i) {
  // assumes the caller knows the size of the structtor.
  assert(ref_is_struct(v));
  assert(i >= 0 && i < struct_len(v));
  Obj* els = struct_els(v);
  return els[i];
}


UNUSED_FN static void struct_put(Obj v, Int i, Obj el) {
  // owns el.
  // assumes the caller knows the size of the structtor.
  assert(ref_is_struct(v));
  assert(i >= 0 && i < struct_len(v));
  Obj* els = struct_els(v);
  // safe to release els[i] first, because even if el is els[i],
  // it is owned by this function so the release could never cause deallocation.
  rc_rel(els[i]);
  els[i] = el;
}


static Obj struct_slice(Obj v, Int f, Int t) {
  // owns v.
  Int l = struct_len(v);
  if (f < 0) f += l;
  if (t < 0) t += l;
  f = int_clamp(f, 0, l);
  t = int_clamp(t, 0, l);
  Int ls = t - f; // length of slice.
  if (ls == l) {
    return v; // no ret/rel necessary.
  }
  Obj s = struct_new_raw(ls);
  Obj* src = struct_els(v);
  Obj* dst = struct_els(s);
  for_in(i, ls) {
    dst[i] = rc_ret(src[i + f]);
  }
  rc_rel(v);
  return s;
}


static Obj struct_slice_from(Obj v, Int f) {
  // owns v.
  return struct_slice(v, f, struct_len(v));
}


static Obj struct_rel_fields(Obj v) {
  Mem m = struct_mem(v);
  if (!m.len) return obj0; // termination sentinel for rc_rel tail loop.
  Int last_i = m.len - 1;
  it_mem_to(it, m, last_i) {
    rc_rel(*it);
  }
  return m.els[last_i];
}


typedef enum {
  vs_struct,
  vs_quo,
  vs_qua,
  vs_unq,
  vs_label,
  vs_variad,
  vs_seq,
} Struct_shape;


static Struct_shape struct_shape(Obj v) {
  assert(ref_is_struct(v));
  Int len = struct_len(v);
  if (!len) return vs_struct;
  Obj e0 = struct_el(v, 0);
  if (len == 2) {
    if (e0.u == s_Quo.u) return vs_quo;
    if (e0.u == s_Qua.u) return vs_qua;
    if (e0.u == s_Unq.u) return vs_unq;
  }
  if (len == 4 && obj_is_sym(struct_el(v, 1))) {
    if (e0.u == s_Label.u) return vs_label;
    if (e0.u == s_Variad.u) return vs_variad;
  }
  if (e0.u == s_Syn_seq.u) return vs_seq;
  return vs_struct;
}


static Bool struct_contains_unquote(Obj v) {
  assert(ref_is_struct(v));
  Mem m = struct_mem(v);
  if (!m.len) return false;
  if (m.els[0].u == s_Unq.u) return true; // struct is an unquote form.
  it_mem_from(it, m, 1) {
    Obj e = *it;
    if (obj_is_struct(e) && struct_contains_unquote(e)) return true;
  }
  return false;
}

