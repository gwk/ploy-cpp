// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-func.h"


static NO_RETURN _exc_raise(Obj env, Chars_const fmt, Chars_const args_src, ...);
// NOTE: the exc macros expect env:Obj to be defined in the current scope.
#define exc_raise(fmt, ...) _exc_raise(env, fmt, #__VA_ARGS__, ##__VA_ARGS__)
#define exc_check(condition, ...) if (!(condition)) exc_raise(__VA_ARGS__)


typedef struct {
  Obj env;
  Obj obj;
} Step;

#define mk_step(e, o) (Step){.env=(e), .obj=(o)}


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
  obj_rel(o);
  return new_bool(b);
}


static Obj host_not(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj arg = args.els[0];
  Bool b = is_true(arg);
  obj_rel(arg);
  return new_bool(!b);
}


static Obj host_ineg(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj n = args.els[0];
  exc_check(obj_is_int(n), "neg requires Int; received: %o", n);
  Int i = int_val(n);
  obj_rel_val(n);
  return new_int(-i);
}


static Obj host_iabs(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj n = args.els[0];
  exc_check(obj_is_int(n), "abs requires Int; received: %o", n);
  Int i = int_val(n);
  obj_rel_val(n);
  return new_int(i < 0 ? -i : i);
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
  obj_rel_val(n0); \
  obj_rel_val(n1); \
  return new_int(i); \
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
  obj_rel_val(a);
  obj_rel_val(b);
  return new_bool(res);
}


static Obj host_Vec(Obj env, Mem args) {
  // owns elements of args.
  return new_vec_M(args);
}


//static Obj host_chain(Obj env, Mem args) {}


static Obj host_len(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj o = args.els[0];
  Int l;
  if (o.u == s_VEC0.u || o.u == blank.u) {
    l = 0;
  } else {
    exc_check(obj_is_data(o) || obj_is_vec(o), "len requires Data or Vec; received: %o", o);
    l = o.rcl->len;
  }
  obj_rel(o);
  return new_int(l);
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
  obj_rel(v);
  obj_rel_val(i);
  return obj_ret(el);
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
  if (v.u == s_VEC0.u) {
    obj_rel(from);
    obj_rel(to);
    return v; // no ret/rel necessary.
  }
  Int l = vec_len(v);
  Int f = int_val(from);
  Int t = int_val(to);
  if (f < 0) f += l;
  if (t < 0) t += l;
  f = int_clamp(f, 0, l);
  t = int_clamp(t, 0, l);
  Int ls = t - f; // length of slice.
  if (ls < 1) {
    obj_rel(v);
    obj_rel(from);
    obj_rel(to);
    return obj_ret_val(s_VEC0);
  }
  Obj s = new_vec_raw(ls);
  Obj* src = vec_ref_els(v);
  Obj* dst = vec_ref_els(s);
  for_in(i, ls) {
    dst[i] = obj_ret(src[i + f]);
  }
  obj_rel(v);
  obj_rel_val(from);
  obj_rel_val(to);
  return s;
}


static Obj host_prepend(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj el = args.els[0];
  Obj vec = args.els[1];
  exc_check(obj_is_vec(vec), "prepend requires arg 2 to be a Vec; received: %o", vec);
  Mem  m = vec_mem(vec);
  Obj res = new_vec_raw(m.len + 1);
  Obj* els = vec_ref_els(res);
  els[0] = el;
  for_in(i, m.len) {
    els[1 + i] = obj_ret(m.els[i]);
  }
  obj_rel(vec);
  return res;
}


static Obj host_append(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj vec = args.els[0];
  Obj el = args.els[1];
  exc_check(obj_is_vec(vec), "append requires arg 1 to be a Vec; received: %o", vec);
  Mem  m = vec_mem(vec);
  Obj res = new_vec_raw(m.len + 1);
  Obj* els = vec_ref_els(res);
  for_in(i, m.len) {
    els[i] = obj_ret(m.els[i]);
  }
  els[m.len] = el;
  obj_rel(vec);
  return res;
}


// TODO: host_cat


static Obj host_raw_write(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj f = args.els[0];
  Obj d = args.els[1];
  exc_check(obj_is_file(f), "write requires arg 1 to be a File; received: %o", f);
  exc_check(obj_is_data(d), "write requires arg 2 to be a Data; received: %o", d);
  CFile file = file_file(f);
  // for now, ignore the return value.
  fwrite(data_ptr(d), size_Char, cast(Uns, data_len(d)), file);
  obj_rel(f);
  obj_rel(d);
  return obj_ret_val(s_void);
}


static Obj host_raw_write_repr(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 2);
  Obj f = args.els[0];
  Obj o = args.els[1];
  exc_check(obj_is_file(f), "write requires arg 1 to be a File; received: %o", f);
  CFile file = file_file(f);
  write_repr(file, o);
  obj_rel(f);
  obj_rel(o);
  return obj_ret_val(s_void);
}


static Obj host_raw_flush(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj f = args.els[0];
  exc_check(obj_is_file(f), "write requires arg 1 to be a File; received: %o", f);
  CFile file = file_file(f);
  fflush(file);
  obj_rel(f);
  return obj_ret_val(s_void);
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
  Step step = run(obj_ret(target_env), code);
  obj_rel(code);
  obj_rel(step.env);
  return step.obj;
}


static Obj host_type_sym(Obj env, Mem args) {
  // owns elements of args.
  assert(args.len == 1);
  Obj o = args.els[0];
  Obj s = obj0;
  switch (obj_tag(o)) {
    case ot_int: s = s_INT; break;
    case ot_sym: s = s_SYM; break;
    case ot_data: s = s_DATA; break;
    case ot_ref:
      switch (ref_struct_tag(o)) {
        case st_Data: s = s_DATA; break;
        case st_Vec: s = s_VEC; break;
        case st_I32: s = s_I32; break;
        case st_I64: s = s_I64; break;
        case st_U32: s = s_U32; break;
        case st_U64: s = s_U64; break;
        case st_F32: s = s_F32; break;
        case st_F64: s = s_F64; break;
        case st_File: s = s_FILE; break;
        case st_Func_host: s = s_FUNC_HOST; break;
        case st_Reserved_A:
        case st_Reserved_B:
        case st_Reserved_C:
        case st_Reserved_D:
        case st_Reserved_E:
        case st_Reserved_F:
          error_obj("bad ref tag:", o);
      }
      break;
  }
  obj_rel(o);
  return obj_ret_val(s);
}


static Obj env_bind(Obj env, Obj sym, Obj func);

static Obj host_init(Obj env) {
  Obj sym, val;

#define DEF_FH(len_pars, f, n) \
sym = new_sym_from_chars(cast(Chars, n)); \
val = new_func_host(sym, len_pars, f); \
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
  DEF_FH(-1, host_Vec, "Vec")
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

#define DEF_FILE(f, string, is_readable, is_writable) \
sym = new_sym_from_chars(cast(Chars, string)); \
val = new_vec2(new_data_from_chars(cast(Chars, "<" string ">")), \
new_file(f, is_readable, is_writable)); \
env = env_bind(env, sym, val);

  DEF_FILE(stdin, "std-in", true, false)
  DEF_FILE(stdout, "std-out", false, true)
  DEF_FILE(stderr, "std-err", false, true)

#undef DEF_FILE

  return env;
}
