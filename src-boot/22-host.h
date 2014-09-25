// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "21-type.h"

#define GET_VAL(v) Obj v = env_get(env, s_##v)
#define GET_A GET_VAL(a)
#define GET_AB GET_A; GET_VAL(b)
#define GET_ABC GET_AB; GET_VAL(c)

static Obj host_identity(Trace* trace, Obj env) {
  GET_A;
  return rc_ret(a);
}


static Obj host_is(Trace* trace, Obj env) {
  GET_AB;
  return bool_new(is(a, b));
}


static Obj host_is_true(Trace* trace, Obj env) {
  GET_A;
  return bool_new(is_true(a));
}


static Obj host_not(Trace* trace, Obj env) {
  GET_A;
  return bool_new(!is_true(a));
}


static Obj host_ineg(Trace* trace, Obj env) {
  GET_A;
  exc_check(obj_is_int(a), "neg requires Int; received: %o", a);
  Int i = int_val(a);
  return int_new(-i);
}


static Obj host_iabs(Trace* trace, Obj env) {
  GET_A;
  exc_check(obj_is_int(a), "abs requires Int; received: %o", a);
  Int i = int_val(a);
  return int_new(i < 0 ? -i : i);
}


// TODO: check for overflow. currently we rely on clang to insert overflow traps.
#define HOST_BIN_OP(op) \
static Obj host_##op(Trace* trace, Obj env) { \
  GET_AB; \
  exc_check(obj_is_int(a), #op " requires arg 1 to be a Int; received: %o", a); \
  exc_check(obj_is_int(b), #op " requires arg 2 to be a Int; received: %o", b); \
  Int i = op(int_val(a), int_val(b)); \
  return int_new(i); \
}


static Int iadd(Int a, Int b)  { return a + b; }
static Int isub(Int a, Int b)  { return a - b; }
static Int imul(Int a, Int b)  { return a * b; }
static Int idiv(Int a, Int b)  { return a / b; }
static Int imod(Int a, Int b)  { return a % b; }
static Int ipow(Int a, Int b)  { return cast(Int, pow(a, b)); } // TODO: check for overflow.
static Int ishl(Int a, Int b)  { return a << b; }
static Int ishr(Int a, Int b)  { return a >> b; }

static Int ieq(Int a, Int b)   { return a == b; }
static Int ine(Int a, Int b)   { return a != b; }
static Int ilt(Int a, Int b)   { return a < b; }
static Int igt(Int a, Int b)   { return a > b; }
static Int ile(Int a, Int b)   { return a <= b; }
static Int ige(Int a, Int b)   { return a >= b; }

HOST_BIN_OP(iadd)
HOST_BIN_OP(isub)
HOST_BIN_OP(imul)
HOST_BIN_OP(idiv)
HOST_BIN_OP(imod)
HOST_BIN_OP(ipow)
HOST_BIN_OP(ishl)
HOST_BIN_OP(ishr)

HOST_BIN_OP(ieq)
HOST_BIN_OP(ine)
HOST_BIN_OP(ilt)
HOST_BIN_OP(igt)
HOST_BIN_OP(ile)
HOST_BIN_OP(ige)


static Obj host_dlen(Trace* trace, Obj env) {
  GET_A;
  Int l;
  if (is(a, blank)) {
    l = 0;
  } else {
    exc_check(obj_is_data(a), "data-len requires Data; received: %o", a);
    l = data_len(a);
  }
  return int_new(l);
}


static Obj host_mlen(Trace* trace, Obj env) {
  GET_A;
  exc_check(obj_is_cmpd(a), "mlen requires Cmpd; received: %o", a);
  Int l = cmpd_len(a);
  return int_new(l);
}


static Obj host_field(Trace* trace, Obj env) {
  GET_AB;
  exc_check(obj_is_cmpd(a), "field requires arg 1 to be a Struct; received: %o", a);
  exc_check(obj_is_int(b), "field requires arg 2 to be a Int; received: %o", b);
  Int l = cmpd_len(a);
  Int i = int_val(b);
  exc_check(i >= 0 && i < l, "field index out of range; index: %i; len: %i", i, l);
  Obj field = cmpd_el(a, i);
  return rc_ret(field);
}


static Obj host_el(Trace* trace, Obj env) {
  GET_AB;
  exc_check(obj_is_cmpd(a), "el requires arg 1 to be a Mem; received: %o", a);
  exc_check(obj_is_int(b), "el requires arg 2 to be a Int; received: %o", b);
  Int l = cmpd_len(a);
  Int i = int_val(b);
  exc_check(i >= 0 && i < l, "el index out of range; index: %i; len: %i", i, l);
  Obj el = cmpd_el(a, i);
  return rc_ret(el);
}


static Obj host_slice(Trace* trace, Obj env) {
  GET_ABC;
  exc_check(obj_is_cmpd(a), "el requires arg 1 to be a Mem; received: %o", a);
  exc_check(obj_is_int(b), "el requires arg 2 to be a Int; received: %o", b);
  exc_check(obj_is_int(c), "el requires arg 3 to be a Int; received: %o", c);
  Int f = int_val(b);
  Int t = int_val(c);
  return cmpd_slice(a, f, t);
}


static Obj host_prepend(Trace* trace, Obj env) {
  GET_AB;
  exc_check(obj_is_cmpd(b), "prepend requires arg 2 to be a Mem; received: %o", b);
  Obj res = cmpd_new_raw(rc_ret(ref_type(b)), cmpd_len(b) + 1);
  cmpd_put(res, 0, rc_ret(a));
  for_in(i, cmpd_len(b)) {
    cmpd_put(res, i + 1, rc_ret(cmpd_el(b, i)));
  }
  return res;
}


static Obj host_append(Trace* trace, Obj env) {
  GET_AB;
  exc_check(obj_is_cmpd(a), "append requires arg 1 to be a Mem; received: %o", a);
  Int l = cmpd_len(a);
  Obj res = cmpd_new_raw(rc_ret(ref_type(a)), l + 1);
  for_in(i, l) {
    cmpd_put(res, i, rc_ret(cmpd_el(a, i)));
  }
  cmpd_put(res, l, rc_ret(b));
  return res;
}


// TODO: host_cat


static Obj host_write(Trace* trace, Obj env) {
  GET_AB;
  exc_check(obj_is_ptr(a), "_host-write requires arg 1 to be a File; received: %o", a);
  exc_check(obj_is_data(b), "_host-write requires arg 2 to be a Data; received: %o", b);
  CFile file = ptr_val(a);
  // for now, ignore the return value.
  fwrite(data_ptr(b), size_Char, cast(Uns, data_len(b)), file);
  return rc_ret_val(s_void);
}


static Obj host_write_repr(Trace* trace, Obj env) {
  GET_AB;
  exc_check(obj_is_ptr(a), "_host-write-repr requires arg 1 to be a File; received: %o", a);
  CFile file = ptr_val(a);
  write_repr(file, b);
  return rc_ret_val(s_void);
}


static Obj host_flush(Trace* trace, Obj env) {
  GET_A;
  exc_check(obj_is_ptr(a), "_host-flush requires arg 1 to be a File; received: %o", a);
  CFile file = ptr_val(a);
  fflush(file);
  return rc_ret_val(s_void);
}


static Obj host_exit(Trace* trace, Obj env) {
  GET_A;
  exc_check(obj_is_int(a), "exit requires arg 1 to be an Int; recived: %o", a);
  exit(cast(I32, int_val(a)));
  // TODO: throw exception to unwind, cleanup, and report counts?
}


static Obj host_error(Trace* trace, Obj env) {
  GET_A;
  exc_raise("error: %o", a);
}


static Obj host_type_of(Trace* trace, Obj env) {
  GET_A;
  Obj t = obj_type(a);
  return rc_ret(t);
}


static void write_data(CFile f, Obj d);

static Obj host_dbg(Trace* trace, Obj env) {
  // owns elements of args.
  GET_AB; // label, obj.
  exc_check(is(obj_type(a), t_Data), "dbg expects argument 1 to be Data: %o", a);
  write_data(stderr, a);
  errFL(": %p rc:%u %o", b, rc_get(b), b);
  return rc_ret_val(s_void);
}


static Obj host_boot_mk_do(Trace* trace, Obj env) {
  GET_A;
  if (!obj_is_cmpd(a)) return a;
  Mem m = cmpd_mem(a);
  Obj val;
  if (m.len == 1) {
    val = rc_ret(m.els[0]);
  } else {
     val = cmpd_new_M_ret(rc_ret(t_Do), m);
  }
  return val;
}


typedef Obj(*Func_host_ptr)(Trace*, Obj);


static Obj host_init_func(Obj env, Int len_pars, Chars_const name, Func_host_ptr ptr) {
  // owns env.
  Obj sym = sym_new_from_chars(name);
  Obj pars; // TODO: add real types; unique value for expression default?
  #define PAR(s) \
  cmpd_new3(rc_ret(t_Par), rc_ret_val(s), rc_ret_val(s_nil), rc_ret_val(s_void))
  switch (len_pars) {
    case 1: pars = cmpd_new1(rc_ret(t_Mem_Par), PAR(s_a)); break;
    case 2: pars = cmpd_new2(rc_ret(t_Mem_Par), PAR(s_a), PAR(s_b)); break;
    case 3: pars = cmpd_new3(rc_ret(t_Mem_Par), PAR(s_a), PAR(s_b), PAR(s_c)); break;
    default: assert(0);
  }
  #undef PAR
  Obj f = cmpd_new8(rc_ret(t_Func),
    rc_ret_val(sym),
    bool_new(false),
    bool_new(false),
    rc_ret(env),
    rc_ret(s_void),
    pars,
    rc_ret_val(s_nil), // TODO: specify actual return type?
    ptr_new(cast(Raw, ptr)));
  return env_bind(env, false, sym, f);
}


static Obj host_init_file(Obj env, Chars_const sym_name, Chars_const name, CFile f, Bool r,
  Bool w) {
  // owns env.
  Obj sym = sym_new_from_chars(sym_name);
  Obj val = cmpd_new4(rc_ret(t_File), data_new_from_chars(name), ptr_new(f),
    bool_new(r), bool_new(w));
  return env_bind(env, false, sym, val);
}


static Obj host_init(Obj env) {
#define DEF_FH(len_pars, n, f) env = host_init_func(env, len_pars, n, f)
  DEF_FH(1, "identity", host_identity);
  DEF_FH(2, "is", host_is);
  DEF_FH(1, "is-true", host_is_true);
  DEF_FH(1, "not", host_not);
  DEF_FH(1, "ineg", host_ineg);
  DEF_FH(1, "iabs", host_iabs);
  DEF_FH(2, "iadd", host_iadd);
  DEF_FH(2, "isub", host_isub);
  DEF_FH(2, "imul", host_imul);
  DEF_FH(2, "idiv", host_idiv);
  DEF_FH(2, "imod", host_imod);
  DEF_FH(2, "ipow", host_ipow);
  DEF_FH(2, "ishl", host_ishl);
  DEF_FH(2, "ishr", host_ishr);
  DEF_FH(2, "ieq", host_ieq);
  DEF_FH(2, "ine", host_ine);
  DEF_FH(2, "ilt", host_ilt);
  DEF_FH(2, "ile", host_ile);
  DEF_FH(2, "igt", host_igt);
  DEF_FH(2, "ige", host_ige);
  DEF_FH(1, "dlen", host_dlen);
  DEF_FH(1, "mlen", host_mlen);
  DEF_FH(2, "field", host_field);
  DEF_FH(2, "el", host_el);
  DEF_FH(3, "slice", host_slice);
  DEF_FH(2, "prepend", host_prepend);
  DEF_FH(2, "append", host_append);
  DEF_FH(2, "_host-write", host_write);
  DEF_FH(2, "_host-write-repr", host_write_repr);
  DEF_FH(1, "_host-flush", host_flush);
  DEF_FH(1, "exit", host_exit);
  DEF_FH(1, "error", host_error);
  DEF_FH(1, "type-of", host_type_of);
  DEF_FH(2, "dbg", host_dbg);
  DEF_FH(1, "_boot-mk-do", host_boot_mk_do);
#undef DEF_FH

#define DEF_FILE(n, f, r, w) env = host_init_file(env, n, "<" n ">", f, r, w);
  
  DEF_FILE("std-in", stdin, true, false)
  DEF_FILE("std-out", stdout, false, true)
  DEF_FILE("std-err", stderr, false, true)
#undef DEF_FILE

  return env;
}
