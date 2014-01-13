// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-func-host.h"


static Obj host_write(Obj f, Obj d) {
  check_obj(ref_is_file(f), "write expected arg 1 File; found", f);
  check_obj(ref_is_data(d), "write expected arg 2 Data; found", d);
  File file = file_file(f);
  Int i = cast(Int, fwrite(data_ptr(d).c, size_Char, cast(Uns, ref_len(d)), file));
  return new_int(i);
}


static Obj host_flush(Obj f) {
  check_obj(ref_is_file(f), "write expected arg 1 File; found", f);
  File file = file_file(f);
  fflush(file);
  return VOID;
}


static Obj host_len(Obj dv) {
  check_obj(ref_is_data(dv) || ref_is_vec(dv), "len expected Data or Vec; found");
  return ref_len(dv);
}


static Obj host_el(Obj v, Obj i) {
  check_obj(obj_is_vec(v), "el expected arg 1 Vec; found");
  check_obj(obj_is_int(i), "el expected arg 2 Int; found");
  Int j = int_val(i);
  Int l = vec_len(v);
  check(j >= 0 && j < l, "el index out of range; index: %ld; len: %ld", j, l);
  return vec_el(v, i);
}


static Obj host_slice(Obj v, Obj i0, Obj i1) {
  check_obj(obj_is_vec(v), "el expected arg 1 Vec; found");
  check_obj(obj_is_int(i), "el expected arg 2 Int; found");
  check_obj(obj_is_int(i), "el expected arg 3 Int; found");
  Int j0 = int_val(i0);
  Int j1 = int_val(i1);
  Int l = vec_len(v);
  if (j0 < 0) j0 += l;
  if (j1 < 0) j1 += l;
  j0 = int_clamp(j0, 0, len - 1);
  j1 = int_clamp(j1, 0, len - 1);
  Int rl = j1 - j0;
  if (rl < 1) return VEC0;
  Mem m = mem_mk(vec_els(v) + j0, rl);
  return vec_new_M(m);
}


static Obj host_add(Obj n0, Obj n1) {
  // TODO: handle flt.
  check_obj(obj_is_int(n0), "add expected arg 1 Int; found");
  check_obj(obj_is_int(n1), "add expected arg 2 Int; found");
  Int i = int_val(n0) + int_val(n1);
  // TODO: check for overflow.
  return new_int(i);
}


static Obj host_sub(Obj n0, Obj n1) {
  return new_int(0);
}


static Obj host_mul(Obj n0, Obj n1) {
  return new_int(0);
}


static Obj host_div(Obj n0, Obj n1) {
  return new_int(0);
}


static Obj host_divi(Obj n0, Obj n1) {
  return new_int(0);
}


static Obj host_mod(Obj n0, Obj n1) {
  return new_int(0);
}


static Obj host_pow(Obj n0, Obj n1) {
  return new_int(0);
}


static Obj host_neg(Obj n) {
  return new_int(0);
}


static Obj host_abs(Obj n) {
  return new_int(0);
}


static Obj host_eq(Obj n0, Obj n1) {
  return FALSE;
}


static Obj host_ne(Obj n0, Obj n1) {
  return FALSE;
}


static Obj host_lt(Obj n0, Obj n1) {
  return FALSE;
}


static Obj host_le(Obj n0, Obj n1) {
  return FALSE;
}


static Obj host_gt(Obj n0, Obj n1) {
  return FALSE;
}


static Obj host_ge(Obj n0, Obj n1) {
  return FALSE;
}


static Obj host_not(Obj n) {
  return FALSE;
}


static Obj host_exit(Obj n) {
  exit(cast(I32, int_val(n)));
}



static Obj host_init() {
  Obj frame = END;
  Obj s, f;


#define DEF_FH(ac, n) \
s = new_sym(ss_from_BC(#n)); f = new_func_host(ac, cast(Ptr, host_##n), s); // TODO: bind f to s

  DEF_FH(1, write)
  DEF_FH(1, flush)
  DEF_FH(1, len)
  DEF_FH(1, el)
  DEF_FH(1, slice)
  DEF_FH(1, add)
  DEF_FH(1, sub)
  DEF_FH(1, mul)
  DEF_FH(1, div)
  DEF_FH(1, divi)
  DEF_FH(1, mod)
  DEF_FH(1, pow)
  DEF_FH(1, neg)
  DEF_FH(1, abs)
  DEF_FH(1, eq)
  DEF_FH(1, ne)
  DEF_FH(1, lt)
  DEF_FH(1, le)
  DEF_FH(1, gt)
  DEF_FH(1, ge)
  DEF_FH(1, not)
  DEF_FH(1, exit)
  
#undef DEF_FH

  return frame;
}


