// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-func.h"


static Obj host_identity(Int len_pars, Obj* args) {
  // owns element of args.
  assert(len_pars == 1);
  return args[0];
}


static Obj host_raw_write(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 2);
  Obj f = args[0];
  Obj d = args[1];
  check_obj(ref_is_file(f), "write expected arg 1 File; found", f);
  check_obj(ref_is_data(d), "write expected arg 2 Data; found", d);
  File file = file_file(f);
  Int i = cast(Int, fwrite(data_ptr(d).c, size_Char, cast(Uns, data_len(d)), file));
  obj_rel(f);
  obj_rel(d);
  return new_int(i);
}


static Obj host_raw_flush(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 1);
  Obj f = args[0];
  check_obj(ref_is_file(f), "write expected arg 1 File; found", f);
  File file = file_file(f);
  fflush(file);
  obj_rel(f);
  return obj_ret_val(VOID);
}


static Obj host_len(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 1);
  Obj o = args[0];
  Int l;
  if (o.u == VEC0.u || o.u == blank.u) {
    l = 0;
  }
  else {
    check_obj(ref_is_data(o) || ref_is_vec(o), "len expected Data or Vec; found", o);
    l = o.rcl->len;
  }
  obj_rel(o);
  return new_int(l);
}


static Obj host_el(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 2);
  Obj v = args[0];
  Obj i = args[1];
  check_obj(obj_is_vec(v), "el expected arg 1 Vec; found", v);
  check_obj(obj_is_int(i), "el expected arg 2 Int; found", i);
  Int j = int_val(i);
  Int l = vec_len(v);
  check(j >= 0 && j < l, "el index out of range; index: %ld; len: %ld", j, l);
  Obj el = vec_el(v, j);
  obj_rel(v);
  obj_rel_val(i);
  return obj_ret(el);
}


static Obj host_slice(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 3);
  Obj v = args[0];
  Obj i0 = args[1];
  Obj i1 = args[2];
  check_obj(obj_is_vec(v), "el expected arg 1 Vec; found", v);
  check_obj(obj_is_int(i0), "el expected arg 2 Int; found", i0);
  check_obj(obj_is_int(i1), "el expected arg 3 Int; found", i1);
  Int j0 = int_val(i0);
  Int j1 = int_val(i1);
  Int l = vec_len(v);
  if (j0 < 0) j0 += l;
  if (j1 < 0) j1 += l;
  j0 = int_clamp(j0, 0, l - 1);
  j1 = int_clamp(j1, 0, l - 1);
  Int ls = j1 - j0;
  if (ls < 1) return obj_ret_val(VEC0);
  Obj s = new_vec_raw(ls);
  Obj* src = vec_els(v);
  Obj* dst = vec_els(s);
  for_in(i, ls) {
    dst[i] = obj_ret(src[i]);
  }
  obj_rel(v);
  obj_rel_val(i0);
  obj_rel_val(i1);
  return s;
}


static Obj host_neg(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 1);
  Obj n = args[0];
  check_obj(obj_is_int(n), "add expected Int; found", n);
  Int i = int_val(n);
  obj_rel_val(n);
  return new_int(-i);
}


static Obj host_abs(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 1);
  Obj n = args[0];
  check_obj(obj_is_int(n), "add expected Int; found", n);
  Int i = int_val(n);
  obj_rel_val(n);
  return new_int(i < 0 ? -i : i);
}


// TODO: handle flt.
// TODO: check for overflow.
// owns elements of args.
#define HOST_BIN_OP(name) \
static Obj host_##name(Int len_pars, Obj* args) { \
  assert(len_pars == 2); \
  Obj n0 = args[0]; \
  Obj n1 = args[1]; \
  check_obj(obj_is_int(n0), "add expected arg 1 Int; found", n0); \
  check_obj(obj_is_int(n1), "add expected arg 2 Int; found", n1); \
  Int i = int_##name(int_val(n0), int_val(n1)); \
  obj_rel_val(n0); \
  obj_rel_val(n1); \
  return new_int(i); \
}


static Int int_add(Int a, Int b)  { return a + b; }
static Int int_sub(Int a, Int b)  { return a - b; }
static Int int_mul(Int a, Int b)  { return a * b; }
static Int int_divi(Int a, Int b) { return a / b; }
static Int int_mod(Int a, Int b)  { return a % b; }
static Int int_pow(Int a, Int b)  { return cast(Int, pow(a, b)); } // TODO: check for overflow.
  
static Int int_eq(Int a, Int b)   { return a == b; }
static Int int_ne(Int a, Int b)   { return a != b; }
static Int int_lt(Int a, Int b)   { return a < b; }
static Int int_gt(Int a, Int b)   { return a > b; }
static Int int_le(Int a, Int b)   { return a <= b; }
static Int int_ge(Int a, Int b)   { return a >= b; }

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


static Obj host_not(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 1);
  Obj b = args[0];
  Obj r = FALSE;
  if (b.u == FALSE.u) r = TRUE;
  else if (b.u != TRUE.u) {
    error_obj("not requires a Bool value; found", b);
  }
  obj_rel_val(b);
  return r;
}


static Obj host_exit(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 1);
  Obj n = args[0];
  exit(cast(I32, int_val(n)));
  // TODO: throw exception to unwind, cleanup, and report counts?
}


static Obj host_Vec(Int len_pars, Obj* args) {
  // owns elements of args.
  return new_vec_M(mem_mk(len_pars, args));
}


//static Obj host_chain(Int len_pars, Obj* args) {}


static Obj run(Obj env, Obj code);

static Obj host_run(Int len_pars, Obj* args) {
  // owns elements of args.
  assert(len_pars == 2);
  Obj env = args[0];
  Obj code = args[1];
  Obj val = run(env, code);
  obj_rel(env);
  obj_rel(code);
  return val;
}


static Obj env_frame_bind(Obj frame, Obj sym, Obj func);

static Obj host_init() {
  Obj frame = obj_ret_val(CHAIN0);
  Obj sym, val;

#define DEF_FH(len_pars, n) \
sym = new_sym(ss_from_BC(#n)); \
val = new_func_host(sym, len_pars, host_##n); \
frame = env_frame_bind(frame, sym, val);

  DEF_FH(1, identity)
  DEF_FH(2, raw_write)
  DEF_FH(1, raw_flush)
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
  DEF_FH(-1, Vec)
  DEF_FH(2, run)
  
#undef DEF_FH

#define DEF_FILE(f, string, is_readable, is_writable) \
sym = new_sym(ss_from_BC(string)); \
val = new_vec2(new_data_from_BC("<" string ">"), new_file(f, is_readable, is_writable)); \
frame = env_frame_bind(frame, sym, val);
  
  DEF_FILE(stdin, "std-in", true, false)
  DEF_FILE(stdout, "std-out", false, true)
  DEF_FILE(stderr, "std-err", false, true)
  
#undef DEF_FILE

  return frame;
}

