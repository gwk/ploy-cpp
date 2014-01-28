// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-func.h"


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
  check_obj(ref_is_data(dv) || ref_is_vec(dv), "len expected Data or Vec; found", dv);
  return new_int(ref_len(dv));
}


static Obj host_el(Obj v, Obj i) {
  check_obj(obj_is_vec(v), "el expected arg 1 Vec; found", v);
  check_obj(obj_is_int(i), "el expected arg 2 Int; found", i);
  Int j = int_val(i);
  Int l = ref_len(v);
  check(j >= 0 && j < l, "el index out of range; index: %ld; len: %ld", j, l);
  return vec_el(v, j);
}


static Obj host_slice(Obj v, Obj i0, Obj i1) {
  check_obj(obj_is_vec(v), "el expected arg 1 Vec; found", v);
  check_obj(obj_is_int(i0), "el expected arg 2 Int; found", i0);
  check_obj(obj_is_int(i1), "el expected arg 3 Int; found", i1);
  Int j0 = int_val(i0);
  Int j1 = int_val(i1);
  Int l = ref_len(v);
  if (j0 < 0) j0 += l;
  if (j1 < 0) j1 += l;
  j0 = int_clamp(j0, 0, l - 1);
  j1 = int_clamp(j1, 0, l - 1);
  Int rl = j1 - j0;
  if (rl < 1) return VEC0;
  Mem m = mem_mk(vec_els(v) + j0, rl);
  return new_vec_M(m);
}


static Obj host_neg(Obj n) {
  check_obj(obj_is_int(n), "add expected Int; found", n);
  Int i = int_val(n);
  return new_int(-i);
}


static Obj host_abs(Obj n) {
  check_obj(obj_is_int(n), "add expected Int; found", n);
  Int i = int_val(n);
  return new_int(i < 0 ? -i : i);
}


// TODO: handle flt.
// TODO: check for overflow.
#define HOST_BIN_OP(name) \
static Obj host_##name(Obj n0, Obj n1) { \
  check_obj(obj_is_int(n0), "add expected arg 1 Int; found", n0); \
  check_obj(obj_is_int(n1), "add expected arg 2 Int; found", n1); \
  Int i = int_##name(int_val(n0), int_val(n1)); \
  return new_int(i); \
}


static Int int_add(Int a, Int b) { return a + b; }
static Int int_sub(Int a, Int b) { return a - b; }
static Int int_mul(Int a, Int b) { return a * b; }
static Int int_divi(Int a, Int b) { return a / b; }
static Int int_mod(Int a, Int b) { return a % b; }
static Int int_pow(Int a, Int b) { return cast(Int, pow(a, b)); }

static Int int_eq(Int a, Int b)  { return a == b; }
static Int int_ne(Int a, Int b)  { return a != b; }
static Int int_lt(Int a, Int b)  { return a < b; }
static Int int_gt(Int a, Int b)  { return a > b; }
static Int int_le(Int a, Int b)  { return a <= b; }
static Int int_ge(Int a, Int b)  { return a >= b; }

HOST_BIN_OP(add)
HOST_BIN_OP(sub)
HOST_BIN_OP(mul)
HOST_BIN_OP(divi)
HOST_BIN_OP(mod)
HOST_BIN_OP(pow)
HOST_BIN_OP(eq)
HOST_BIN_OP(ne)
HOST_BIN_OP(lt)
HOST_BIN_OP(gt)
HOST_BIN_OP(le)
HOST_BIN_OP(ge)


static Obj host_not(Obj b) {
  if (b.u == FALSE.u) return TRUE;
  if (b.u == TRUE.u) return FALSE;
  error_obj("not requires a Bool value; found", b);
}


static Obj host_exit(Obj n) {
  exit(cast(I32, int_val(n)));
}


static Obj run(Obj env, Obj code);

static Obj host_run(Obj env, Obj code) {
  return run(env, code);
}


static Obj env_frame_bind(Obj frame, Obj sym, Obj func);

static Obj host_init() {
  Obj frame = END;
  Obj s, f;

#define DEF_FH(ac, n) \
s = new_sym(ss_from_BC(#n)); \
f = new_func_host(st_Func_host_##ac, s, cast(Ptr, host_##n)); \
frame = env_frame_bind(frame, s, f);

  DEF_FH(2, write)
  DEF_FH(1, flush)
  DEF_FH(1, len)
  DEF_FH(2, el)
  DEF_FH(3, slice)
  DEF_FH(1, neg)
  DEF_FH(1, abs)
  DEF_FH(2, add)
  DEF_FH(2, sub)
  DEF_FH(2, mul)
  DEF_FH(2, divi)
  DEF_FH(2, mod)
  DEF_FH(2, pow)
  DEF_FH(2, eq)
  DEF_FH(2, ne)
  DEF_FH(2, lt)
  DEF_FH(2, le)
  DEF_FH(2, gt)
  DEF_FH(2, ge)
  DEF_FH(1, not)
  DEF_FH(1, exit)
  DEF_FH(2, run)
  
#undef DEF_FH

  return frame;
}

