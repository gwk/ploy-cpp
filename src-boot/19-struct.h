// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-env.h"


struct _Struct {
  Obj type;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Struct);


static Obj struct_new_raw(Obj type, Int len) {
  // owns type.
  Obj s = ref_alloc(rt_Struct, size_Struct + (size_Obj * len));
  s.s->type = type;
  s.s->len = len;
  return s;
}


static Obj struct_new_M(Obj type, Mem m) {
  // owns type, elements of m.
  Obj s = struct_new_raw(type, m.len);
  Obj* els = struct_els(s);
  for_in(i, m.len) {
    els[i] = mem_el_move(m, i);
  }
  return s;
}


static Obj struct_new_EM(Obj type, Obj el, Mem m) {
  // owns type, el, elements of m.
  Int len = m.len + 1;
  Obj s = struct_new_raw(type, len);
  Obj* els = struct_els(s);
  els[0] = el;
  for_in(i, m.len) {
    els[i + 1] = mem_el_move(m, i);
  }
  return s;
}


static Obj struct_new1(Obj type, Obj a) {
  // owns all arguments.
  Obj s = struct_new_raw(type, 1);
  Obj* els = struct_els(s);
  els[0] = a;
  return s;
}


static Obj struct_new2(Obj type, Obj a, Obj b) {
  // owns all arguments.
  Obj s = struct_new_raw(type, 2);
  Obj* els = struct_els(s);
  els[0] = a;
  els[1] = b;
  return s;
}


static Obj struct_new3(Obj type, Obj a, Obj b, Obj c) {
  // owns all arguments.
  Obj s = struct_new_raw(type, 3);
  Obj* els = struct_els(s);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  return s;
}


static Obj struct_new4(Obj type, Obj a, Obj b, Obj c, Obj d) {
  // owns all arguments.
  Obj s = struct_new_raw(type, 4);
  Obj* els = struct_els(s);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  els[3] = d;
  return s;
}


static Int struct_len(Obj s) {
  assert(ref_is_struct(s));
  assert(s.s->len >= 0);
  return s.s->len;
}


static Obj* struct_els(Obj s) {
  assert(ref_is_struct(s));
  return cast(Obj*, s.s + 1); // address past struct header.
}


static Mem struct_mem(Obj s) {
  return mem_mk(struct_len(s), struct_els(s));
}


static Obj struct_el(Obj s, Int i) {
  // assumes the caller knows the size of the structtor.
  assert(ref_is_struct(s));
  assert(i >= 0 && i < struct_len(s));
  Obj* els = struct_els(s);
  return els[i];
}


UNUSED_FN static void struct_put(Obj s, Int i, Obj el) {
  // owns el.
  // assumes the caller knows the size of the structtor.
  assert(ref_is_struct(s));
  assert(i >= 0 && i < struct_len(s));
  Obj* els = struct_els(s);
  // safe to release els[i] first, because even if el is els[i],
  // it is owned by this function so the release could never cause deallocation.
  rc_rel(els[i]);
  els[i] = el;
}


static Obj struct_slice(Obj s, Int f, Int t) {
  // owns s.
  assert(ref_is_struct(s));
  Int l = struct_len(s);
  if (f < 0) f += l;
  if (t < 0) t += l;
  f = int_clamp(f, 0, l);
  t = int_clamp(t, 0, l);
  Int ls = t - f; // length of slice.
  if (ls == l) {
    return s; // no ret/rel necessary.
  }
  Obj slice = struct_new_raw(rc_ret(ref_type(s)), ls);
  Obj* src = struct_els(s);
  Obj* dst = struct_els(slice);
  for_in(i, ls) {
    dst[i] = rc_ret(src[i + f]);
  }
  rc_rel(s);
  return slice;
}


static Obj struct_slice_from(Obj s, Int f) {
  // owns s.
  return struct_slice(s, f, struct_len(s));
}


static Obj struct_rel_fields(Obj s) {
  Mem m = struct_mem(s);
  if (!m.len) return obj0; // termination sentinel for rc_rel tail loop.
  Int last_i = m.len - 1;
  it_mem_to(it, m, last_i) {
    rc_rel(*it);
  }
  return m.els[last_i];
}


typedef enum {
  ss_struct,
  ss_quo,
  ss_qua,
  ss_unq,
  ss_label,
  ss_variad,
  ss_seq,
} Struct_shape;


static Struct_shape struct_shape(Obj s) {
  assert(ref_is_struct(s));
  Int len = struct_len(s);
  if (!len) return ss_struct;
  Obj e0 = struct_el(s, 0);
  if (len == 2) {
    if (is(e0, s_Quo)) return ss_quo;
    if (is(e0, s_Qua)) return ss_qua;
    if (is(e0, s_Unq)) return ss_unq;
  }
  if (len == 4 && obj_is_sym(struct_el(s, 1))) {
    if (is(e0, s_Label)) return ss_label;
    if (is(e0, s_Variad)) return ss_variad;
  }
  if (is(e0, s_Syn_seq)) return ss_seq;
  return ss_struct;
}


static Bool struct_contains_unquote(Obj s) {
  assert(ref_is_struct(s));
  Mem m = struct_mem(s);
  if (!m.len) return false;
  if (m.els[0].u == s_Unq.u) return true; // struct is an unquote form.
  it_mem_from(it, m, 1) {
    Obj e = *it;
    if (obj_is_struct(e) && struct_contains_unquote(e)) return true;
  }
  return false;
}

