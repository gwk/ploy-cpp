// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "19-env.h"


struct _Struct {
  Ref_head head;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Struct);


static Obj struct_new_raw(Obj type, Int len) {
  // owns type.
  Obj s = ref_new(size_Struct + (size_Obj * len), type, false);
  s.s->len = len;
  return s;
}


static Obj* struct_els(Obj s);

static Obj struct_new_M(Obj type, Mem m) {
  // owns type, elements of m.
  Obj s = struct_new_raw(type, m.len);
  Obj* els = struct_els(s);
  for_in(i, m.len) {
    els[i] = mem_el_move(m, i);
  }
  return s;
}


static Obj struct_new_M_ret(Obj type, Mem m) {
  // owns type.
  Obj s = struct_new_raw(type, m.len);
  Obj* els = struct_els(s);
  for_in(i, m.len) {
    els[i] = mem_el_ret(m, i);
  }
  return s;
}


UNUSED_FN static Obj struct_new_EM(Obj type, Obj el, Mem m) {
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


static Obj struct_new6(Obj type, Obj a, Obj b, Obj c, Obj d, Obj e, Obj f) {
  // owns all arguments.
  Obj s = struct_new_raw(type, 6);
  Obj* els = struct_els(s);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  els[3] = d;
  els[4] = e;
  els[5] = f;
  return s;
}


static Obj struct_new7(Obj type, Obj a, Obj b, Obj c, Obj d, Obj e, Obj f, Obj g) {
  // owns all arguments.
  Obj s = struct_new_raw(type, 7);
  Obj* els = struct_els(s);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  els[3] = d;
  els[4] = e;
  els[5] = f;
  els[6] = g;
  return s;
}


static Obj struct_new8(Obj type, Obj a, Obj b, Obj c, Obj d, Obj e, Obj f, Obj g, Obj h) {
  // owns all arguments.
  Obj s = struct_new_raw(type, 8);
  Obj* els = struct_els(s);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  els[3] = d;
  els[4] = e;
  els[5] = f;
  els[6] = g;
  els[7] = h;
  return s;
}


static Obj struct_new14(Obj type, Obj a, Obj b, Obj c, Obj d, Obj e, Obj f, Obj g, Obj h,
  Obj i, Obj j, Obj k, Obj l, Obj m, Obj n) {
  // owns all arguments.
  Obj s = struct_new_raw(type, 14);
  Obj* els = struct_els(s);
  els[0] = a;
  els[1] = b;
  els[2] = c;
  els[3] = d;
  els[4] = e;
  els[5] = f;
  els[6] = g;
  els[7] = h;
  els[8] = i;
  els[9] = j;
  els[10] = k;
  els[11] = l;
  els[12] = m;
  els[13] = n;
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
  // assumes the caller knows the size of the struct.
  assert(ref_is_struct(s));
  assert(i >= 0 && i < struct_len(s));
  Obj* els = struct_els(s);
  return els[i];
}


static Obj struct_slice(Obj s, Int f, Int t) {
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
  return slice;
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


static Obj t_Unq;

static Bool struct_contains_unquote(Obj s) {
  assert(ref_is_struct(s));
  if (is(obj_type(s), t_Unq)) return true;
  Mem m = struct_mem(s);
  it_mem(it, m) {
    Obj e = *it;
    if (obj_is_struct(e) && struct_contains_unquote(e)) return true;
  }
  return false;
}


// iterate over a struct v.
#define it_struct(it, v) \
for (Obj *it = struct_els(v), *_end_##it = it + struct_len(v); \
it < _end_##it; \
it++)

