// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "21-func.h"


static NO_RETURN _exc_raise(Obj env, Chars_const fmt, Chars_const args_src, ...);
// NOTE: the exc macros expect env:Obj to be defined in the current scope.
#define exc_raise(fmt, ...) _exc_raise(env, fmt, #__VA_ARGS__, ##__VA_ARGS__)
#define exc_check(condition, ...) if (!(condition)) exc_raise(__VA_ARGS__)


static Obj host_identity(Obj env, Mem args) {
  // owns element of args.
  assert(args.len == 1);
  return args.els[0];
}


static Obj host_is_true(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj o = args.els[0];
  Bool b = is_true(o);
  rc_rel(o);
  return bool_new(b);
}


static Obj host_not(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj arg = args.els[0];
  Bool b = is_true(arg);
  rc_rel(arg);
  return bool_new(!b);
}


static Obj host_ineg(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj n = args.els[0];
  exc_check(obj_is_int(n), "neg requires Int; received: %o", n);
  Int i = int_val(n);
  rc_rel_val(n);
  return int_new(-i);
}


static Obj host_iabs(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj n = args.els[0];
  exc_check(obj_is_int(n), "abs requires Int; received: %o", n);
  Int i = int_val(n);
  rc_rel_val(n);
  return int_new(i < 0 ? -i : i);
}


// TODO: handle flt.
// TODO: check for overflow.
// owns elements of args.
#define HOST_BIN_OP(name) \
static Obj host_##name(Obj env, Mem args) { \
  assert(args.len == 2); \
  Obj n0 = args.els[0]; \
  Obj n1 = args.els[1]; \
  exc_check(obj_is_int(n0), #name " requires arg 1 to be a Int; received: %o", n0); \
  exc_check(obj_is_int(n1), #name " requires arg 2 to be a Int; received: %o", n1); \
  Int i = name(int_val(n0), int_val(n1)); \
  rc_rel_val(n0); \
  rc_rel_val(n1); \
  return int_new(i); \
}


static Int iadd(Int a, Int b)  { return a + b; }
static Int isub(Int a, Int b)  { return a - b; }
static Int imul(Int a, Int b)  { return a * b; }
static Int idiv(Int a, Int b) { return a / b; }
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


static Obj host_sym_eq(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj a = args.els[0];
  Obj b = args.els[1];
  exc_check(obj_is_sym(a), "sym-eq requires argument 1 to be a Sym; received: %o", a);
  exc_check(obj_is_sym(b), "sym-eq requires argument 2 to be a Sym; received: %o", b);
  Bool res = (a.u == b.u);
  rc_rel_val(a);
  rc_rel_val(b);
  return bool_new(res);
}


static Obj host_mk_vec(Obj env, Mem args) {
  // owns elements of args.
  return vec_new_M(args);
}


//static Obj host_chain(Obj env, Mem args) {}


static Obj host_len(Obj env, Mem args) {
  // owns elements of args.
  // TODO: separate host_vec_len and host data_len.
  assert(args.len == 1);
  Obj o = args.els[0];
  Int l;
  if (o.u == s_VEC0.u || o.u == blank.u) {
    l = 0;
  } else {
    exc_check(obj_is_data(o) || obj_is_vec(o), "len requires Data or Vec; received: %o", o);
    l = o.data->len;
  }
  rc_rel(o);
  return int_new(l);
}


static Obj host_el(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj v = args.els[0];
  Obj i = args.els[1];
  exc_check(obj_is_vec(v), "el requires arg 1 to be a Vec; received: %o", v);
  exc_check(obj_is_int(i), "el requires arg 2 to be a Int; received: %o", i);
  Int j = int_val(i);
  exc_check(v.u != s_VEC0.u && v.u != s_CHAIN0.u,
    "el index out of range; index: %i; vec: %o", j, v);
  Int l = vec_ref_len(v);
  exc_check(j >= 0 && j < l, "el index out of range; index: %i; len: %i", j, l);
  Obj el = vec_ref_el(v, j);
  rc_rel(v);
  rc_rel_val(i);
  return rc_ret(el);
}


static Obj host_slice(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 3);
  Obj v = args.els[0];
  Obj from = args.els[1];
  Obj to = args.els[2];
  exc_check(obj_is_vec(v), "el requires arg 1 to be a Vec; received: %o", v);
  exc_check(obj_is_int(from), "el requires arg 2 to be a Int; received: %o", from);
  exc_check(obj_is_int(to), "el requires arg 3 to be a Int; received: %o", to);
  Int f = int_val(from);
  Int t = int_val(to);
  rc_rel_val(from);
  rc_rel_val(to);
  return vec_slice(v, f, t);
}


static Obj host_prepend(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj el = args.els[0];
  Obj vec = args.els[1];
  exc_check(obj_is_vec(vec), "prepend requires arg 2 to be a Vec; received: %o", vec);
  Mem  m = vec_mem(vec);
  Obj res = vec_new_raw(m.len + 1);
  Obj* els = vec_ref_els(res);
  els[0] = el;
  for_in(i, m.len) {
    els[1 + i] = rc_ret(m.els[i]);
  }
  rc_rel(vec);
  return res;
}


static Obj host_append(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj vec = args.els[0];
  Obj el = args.els[1];
  exc_check(obj_is_vec(vec), "append requires arg 1 to be a Vec; received: %o", vec);
  Mem  m = vec_mem(vec);
  Obj res = vec_new_raw(m.len + 1);
  Obj* els = vec_ref_els(res);
  for_in(i, m.len) {
    els[i] = rc_ret(m.els[i]);
  }
  els[m.len] = el;
  rc_rel(vec);
  return res;
}


// TODO: host_cat


static Obj host_raw_write(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj f = args.els[0];
  Obj d = args.els[1];
  exc_check(obj_is_ptr(f), "write requires arg 1 to be a File; received: %o", f);
  exc_check(obj_is_data(d), "write requires arg 2 to be a Data; received: %o", d);
  CFile file = ptr_val(f);
  // for now, ignore the return value.
  fwrite(data_ptr(d), size_Char, cast(Uns, data_len(d)), file);
  rc_rel(f);
  rc_rel(d);
  return rc_ret_val(s_void);
}


static Obj host_raw_write_repr(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj f = args.els[0];
  Obj o = args.els[1];
  exc_check(obj_is_ptr(f), "write requires arg 1 to be a File; received: %o", f);
  CFile file = ptr_val(f);
  write_repr(file, o);
  rc_rel(f);
  rc_rel(o);
  return rc_ret_val(s_void);
}


static Obj host_raw_flush(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj f = args.els[0];
  exc_check(obj_is_ptr(f), "write requires arg 1 to be a File; received: %o", f);
  CFile file = ptr_val(f);
  fflush(file);
  rc_rel(f);
  return rc_ret_val(s_void);
}


static Obj host_exit(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj code = args.els[0];
  exit(cast(I32, int_val(code)));
  // TODO: throw exception to unwind, cleanup, and report counts?
}


static Obj host_error(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj msg = args.els[0];
  exc_raise("error: %o", msg);
}


static Step run(Obj env, Obj code);

static Obj host_run(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj target_env = args.els[0];
  Obj code = args.els[1];
  Step step = run(rc_ret(target_env), code);
  rc_rel(code);
  rc_rel(step.env);
  return step.val;
}


static Obj host_type_sym(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj o = args.els[0];
  Obj s = obj0;
  switch (obj_tag(o)) {
    case ot_ref: s = ref_type_sym(o); break;
    case ot_ptr: s = s_Ptr; break;
    case ot_int: s = s_Int; break;
    case ot_sym: s = (obj_is_data_word(o) ? s_Data_word : s_Sym); break;
  }
  rc_rel(o);
  return rc_ret_val(s);
}


static Obj host_init(Obj env) {
  Obj sym, val;

#define DEF_FH(len_pars, f, n) \
sym = sym_new_from_chars(cast(Chars, n)); \
val = func_host_new(sym, len_pars, f); \
env = env_bind(env, sym, val);

  DEF_FH(1, host_identity, "identity")
  DEF_FH(1, host_is_true, "is-true")
  DEF_FH(1, host_not, "not")
  DEF_FH(1, host_ineg, "ineg")
  DEF_FH(1, host_iabs, "iabs")
  DEF_FH(2, host_iadd, "iadd")
  DEF_FH(2, host_isub, "isub")
  DEF_FH(2, host_imul, "imul")
  DEF_FH(2, host_idiv, "idiv")
  DEF_FH(2, host_imod, "imod")
  DEF_FH(2, host_ipow, "ipow")
  DEF_FH(2, host_ishl, "ishl")
  DEF_FH(2, host_ishr, "ishr")
  DEF_FH(2, host_ieq, "ieq")
  DEF_FH(2, host_ine, "ine")
  DEF_FH(2, host_ilt, "ilt")
  DEF_FH(2, host_ile, "ile")
  DEF_FH(2, host_igt, "igt")
  DEF_FH(2, host_ige, "ige")
  DEF_FH(2, host_sym_eq, "sym-eq")
  DEF_FH(-1, host_mk_vec, "mk-vec")
  DEF_FH(1, host_len, "len")
  DEF_FH(2, host_el, "el")
  DEF_FH(3, host_slice, "slice")
  DEF_FH(2, host_prepend, "prepend")
  DEF_FH(2, host_append, "append")
  DEF_FH(2, host_raw_write, "raw-write")
  DEF_FH(2, host_raw_write_repr, "raw-write-repr")
  DEF_FH(1, host_raw_flush, "raw-flush")
  DEF_FH(1, host_exit, "exit")
  DEF_FH(1, host_error, "error")
  DEF_FH(2, host_run, "run")
  DEF_FH(1, host_type_sym, "type-sym")

#undef DEF_FH

#define DEF_FILE(string, f, is_readable, is_writable) \
sym = sym_new_from_chars(cast(Chars, string)); \
val = vec_new4(data_new_from_chars(cast(Chars, "<" string ">")), \
ptr_new(f), \
bool_new(is_readable), \
bool_new(is_writable)); \
env = env_bind(env, sym, val);

  DEF_FILE("std-in", stdin, true, false)
  DEF_FILE("std-out", stdout, false, true)
  DEF_FILE("std-err", stderr, false, true)

#undef DEF_FILE

  return env;
}
