// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "24-parse.h"

#define GET_VAL(v) Obj v = env_get(env, s_##v)
#define GET_A GET_VAL(a)
#define GET_AB GET_A; GET_VAL(b)
#define GET_ABC GET_AB; GET_VAL(c)

static Obj host_identity(UNUSED Trace* t, Obj env) {
  GET_A;
  return a.ret();
}


static Obj host_is(UNUSED Trace* t, Obj env) {
  GET_AB;
  return Obj::with_Bool(a == b);
}


static Obj host_is_ref(UNUSED Trace* t, Obj env) {
  GET_A;
  return Obj::with_Bool(a.is_ref());
}


static Obj host_is_true(UNUSED Trace* t, Obj env) {
  GET_A;
  return Obj::with_Bool(a.is_true());
}


static Obj host_not(UNUSED Trace* t, Obj env) {
  GET_A;
  return Obj::with_Bool(!a.is_true());
}


static Obj host_id_hash(UNUSED Trace* t, Obj env) {
  GET_A;
  return Obj::with_Int(a.id_hash());
}


static Obj host_ineg(Trace* t, Obj env) {
  GET_A;
  exc_check(a.is_int(), "ineg requires Int; received: %o", a);
  Int i = a.int_val();
  return Obj::with_Int(-i);
}


static Obj host_iabs(Trace* t, Obj env) {
  GET_A;
  exc_check(a.is_int(), "iabs requires Int; received: %o", a);
  Int i = a.int_val();
  return Obj::with_Int(i < 0 ? -i : i);
}


// TODO: check for overflow. currently we rely on clang to insert overflow traps.
#define HOST_BIN_OP(type_name, op) \
static Obj host_##op(Trace* t, Obj env) { \
  GET_AB; \
  exc_check(a.is_int(), #op " requires arg 1 to be a Int; received: %o", a); \
  exc_check(b.is_int(), #op " requires arg 2 to be a Int; received: %o", b); \
  Int i = op(a.int_val(), b.int_val()); \
  return Obj::with_##type_name(i); \
}


static Int iadd(Int a, Int b)  { return a + b; }
static Int isub(Int a, Int b)  { return a - b; }
static Int imul(Int a, Int b)  { return a * b; }
static Int idiv(Int a, Int b)  { return a / b; }
static Int imod(Int a, Int b)  { return (a % b + b) % b; }
static Int ipow(Int a, Int b)  { return Int(pow(a, b)); } // TODO: check for overflow.
static Int ishl(Int a, Int b)  { return a << b; }
static Int ishr(Int a, Int b)  { return a >> b; }

static Int ieq(Int a, Int b)   { return a == b; }
static Int ine(Int a, Int b)   { return a != b; }
static Int ilt(Int a, Int b)   { return a < b; }
static Int igt(Int a, Int b)   { return a > b; }
static Int ile(Int a, Int b)   { return a <= b; }
static Int ige(Int a, Int b)   { return a >= b; }

HOST_BIN_OP(Int, iadd)
HOST_BIN_OP(Int, isub)
HOST_BIN_OP(Int, imul)
HOST_BIN_OP(Int, idiv)
HOST_BIN_OP(Int, imod)
HOST_BIN_OP(Int, ipow)
HOST_BIN_OP(Int, ishl)
HOST_BIN_OP(Int, ishr)

HOST_BIN_OP(Bool, ieq)
HOST_BIN_OP(Bool, ine)
HOST_BIN_OP(Bool, ilt)
HOST_BIN_OP(Bool, igt)
HOST_BIN_OP(Bool, ile)
HOST_BIN_OP(Bool, ige)


static Obj host_dlen(Trace* t, Obj env) {
  GET_A;
  Int l;
  if (a == blank) {
    l = 0;
  } else {
    exc_check(a.is_data(), "dlen requires Data; received: %o", a);
    l = a.data_len();
  }
  return Obj::with_Int(l);
}


static Obj host_cmpd_len(Trace* t, Obj env) {
  GET_A;
  exc_check(a.is_cmpd(), "cmpd-len requires Cmpd; received: %o", a);
  Int l = a.cmpd_len();
  return Obj::with_Int(l);
}


static Obj host_data_ref_iso(Trace* t, Obj env) {
  GET_AB;
  exc_check(a.is_data_ref(), "data-ref-iso requires arg 1 to be a Data ref; received: %o",
    a);
  exc_check(a.is_data_ref(), "data-ref-iso requires arg 1 to be a Data ref; received: %o",
    a);
  return Obj::with_Bool(a.data_ref_iso(b));
}


static Obj host_cmpd_field(Trace* t, Obj env) {
  GET_AB;
  exc_check(a.is_cmpd(), "field requires arg 1 to be a Cmpd; received: %o", a);
  exc_check(b.is_int(), "field requires arg 2 to be an Int; received: %o", b);
  Int l = a.cmpd_len();
  Int i = b.int_val();
  exc_check(i >= 0 && i < l, "field index out of range; index: %i; len: %i", i, l);
  Obj field = a.cmpd_el(i);
  return field.ret();
}


static Obj host_ael(Trace* t, Obj env) {
  GET_AB;
  exc_check(a.is_cmpd(), "ael requires arg 1 to be an Arr; received: %o", a);
  exc_check(b.is_int(), "ael requires arg 2 to be a Int; received: %o", b);
  Int l = a.cmpd_len();
  Int i = b.int_val();
  exc_check(i >= 0 && i < l, "ael index out of range; index: %i; len: %i", i, l);
  Obj el = a.cmpd_el(i);
  return el.ret();
}


static Obj host_anew(Trace* t, Obj env) {
  GET_AB;
  exc_check(a.is_type(), "anew requires arg 1 to be a Type; received: %o", a);
  exc_check(b.is_int(), "anew requires arg 2 to be an Int; received: %o", b);
  Int len = b.int_val();
  Obj res = Obj::Cmpd_raw(a.ret(), len);
  for_in(i, len) {
    res.cmpd_put(i, s_UNINIT.ret_val());
  }
  return res;
}


static Obj host_aput(Trace* t, Obj env) {
  GET_ABC;
  exc_check(a.is_cmpd(), "el requires arg 1 to be a Arr; received: %o", a);
  exc_check(b.is_int(), "el requires arg 2 to be a Int; received: %o", b);
  Int l = a.cmpd_len();
  Int i = b.int_val();
  exc_check(i >= 0 && i < l, "el index out of range; index: %i; len: %i", i, l);
  a.cmpd_el_move(i).rel();
  a.cmpd_put(i, c.ret());
  return a.ret();
}


static Obj host_aslice(Trace* t, Obj env) {
  GET_ABC;
  exc_check(a.is_cmpd(), "el requires arg 1 to be a Arr; received: %o", a);
  exc_check(b.is_int(), "el requires arg 2 to be a Int; received: %o", b);
  exc_check(c.is_int(), "el requires arg 3 to be a Int; received: %o", c);
  Int fr = b.int_val();
  Int to = c.int_val();
  return a.cmpd_slice(fr, to);
}


static Obj host_write(Trace* t, Obj env) {
  GET_AB;
  exc_check(a.is_ptr(), "write requires arg 1 to be a File; received: %o", a);
  exc_check(b.is_data(), "write requires arg 2 to be a Data; received: %o", b);
  CFile file = CFile(a.ptr());
  // for now, ignore the return value.
  fwrite(b.data_chars(), size_Char, Uns(b.data_len()), file);
  return s_void.ret_val();
}


static Obj host_write_repr(Trace* t, Obj env) {
  GET_AB;
  exc_check(a.is_ptr(), "write-repr requires arg 1 to be a File; received: %o", a);
  CFile file = CFile(a.ptr());
  write_repr(file, b);
  return s_void.ret_val();
}


static Obj host_flush(Trace* t, Obj env) {
  GET_A;
  exc_check(a.is_ptr(), "flush requires arg 1 to be a File; received: %o", a);
  CFile file = CFile(a.ptr());
  fflush(file);
  return s_void.ret_val();
}


static Obj host_exit(Trace* t, Obj env) {
  GET_A;
  exc_check(a.is_int(), "exit requires arg 1 to be an Int; recived: %o", a);
  exit(I32(a.int_val()));
  // TODO: throw exception to unwind, cleanup, and report counts?
}


static Obj host_raise(Trace* t, Obj env) {
  GET_A;
  exc_raise("raised: %o", a);
}


static Obj host_type_of(UNUSED Trace* t, Obj env) {
  GET_A;
  Obj type = a.type();
  return type.ret();
}


static Obj host_globalize(UNUSED Trace* t, Obj env) {
  GET_A;
  global_push(a);
  return s_void.ret_val();
}


static Obj host_dbg(Trace* t, Obj env) {
  GET_AB; // label, obj.
  exc_check(a.type() == t_Data, "dbg expects argument 1 to be Data: %o", a);
  write_data(stderr, a);
  errFL(": %p rc:%u %o", b, b.rc(), b);
  return s_void.ret_val();
}


typedef Obj(*Func_host_ptr)(Trace*, Obj);


static Obj host_init_const(Obj env, Chars name, Obj val) {
  return env_bind(env, false, Obj::Sym_from_c(name), val);
}


static Obj host_init_func(Obj env, Int len_pars, Chars name, Func_host_ptr ptr) {
  // owns env.
  Obj sym = Obj::Sym_from_c(name);
  Obj pars; // TODO: add real types; unique value for expression default?
  #define PAR(s) \
  Obj::Cmpd(t_Par.ret(), s.ret_val(), s_nil.ret_val(), s_void.ret_val())
  switch (len_pars) {
    case 1: pars = Obj::Cmpd(t_Arr_Par.ret(), PAR(s_a)); break;
    case 2: pars = Obj::Cmpd(t_Arr_Par.ret(), PAR(s_a), PAR(s_b)); break;
    case 3: pars = Obj::Cmpd(t_Arr_Par.ret(), PAR(s_a), PAR(s_b), PAR(s_c)); break;
    default: assert(0);
  }
  #undef PAR
  Obj f = Obj::Cmpd(t_Func.ret(),
    Obj::with_Bool(false),
    Obj::with_Bool(false),
    env.ret(),
    s_void.ret_val(),
    s_void.ret_val(),
    pars,
    s_nil.ret_val(), // TODO: specify actual return type?
    Obj::with_Ptr(Raw(ptr)));
  return env_bind(env, false, sym, f);
}


static Obj host_init_file(Obj env, Chars sym_name, Chars name, CFile f, Bool r,
  Bool w) {
  // owns env.
  Obj sym = Obj::Sym(sym_name);
  Obj val = Obj::Cmpd(t_File.ret(), Obj::Data(name), Obj::with_Ptr(f),
    Obj::with_Bool(r), Obj::with_Bool(w));
  return env_bind(env, false, sym, val);
}


static Obj host_init(Obj env) {

#define DEF_CONST(c) env = host_init_const(env, #c, Obj::with_Int(c))
  DEF_CONST(OPTION_REC_LIMIT);
#undef DEF_CONST

#define DEF_FH(len_pars, n) env = host_init_func(env, len_pars, #n, host_##n)
  DEF_FH(1, identity);
  DEF_FH(2, is);
  DEF_FH(1, is_ref);
  DEF_FH(1, is_true);
  DEF_FH(1, not);
  DEF_FH(1, id_hash);
  DEF_FH(1, ineg);
  DEF_FH(1, iabs);
  DEF_FH(2, iadd);
  DEF_FH(2, isub);
  DEF_FH(2, imul);
  DEF_FH(2, idiv);
  DEF_FH(2, imod);
  DEF_FH(2, ipow);
  DEF_FH(2, ishl);
  DEF_FH(2, ishr);
  DEF_FH(2, ieq);
  DEF_FH(2, ine);
  DEF_FH(2, ilt);
  DEF_FH(2, ile);
  DEF_FH(2, igt);
  DEF_FH(2, ige);
  DEF_FH(1, dlen);
  DEF_FH(1, cmpd_len);
  DEF_FH(2, data_ref_iso);
  DEF_FH(2, cmpd_field);
  DEF_FH(2, ael);
  DEF_FH(2, anew);
  DEF_FH(3, aput);
  DEF_FH(3, aslice);
  DEF_FH(2, write);
  DEF_FH(2, write_repr);
  DEF_FH(1, flush);
  DEF_FH(1, exit);
  DEF_FH(1, raise);
  DEF_FH(1, type_of);
  DEF_FH(1, globalize);
  DEF_FH(2, dbg);
#undef DEF_FH

#define DEF_FILE(n, f, r, w) env = host_init_file(env, n, "<" n ">", f, r, w);
  DEF_FILE("std-in", stdin, true, false)
  DEF_FILE("std-out", stdout, false, true)
  DEF_FILE("std-err", stderr, false, true)
#undef DEF_FILE

  return env;
}
