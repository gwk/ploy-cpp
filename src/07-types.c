// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "06-Array.c"


typedef enum {
  si_F,
  si_T,
  si_E,
  si_N,
  si_VOID,
  si_QUO,
  si_QUA,
  si_DO,
  si_SCOPE,
  si_LET,
  si_IF,
  si_FN,
  si_EXPA,
} Sym_index;


static const Int shift_sym = width_tag + 1; // 1 bit reserved for data-word flag.
static const Int max_sym_index = 1L << (size_Int * 8 - shift_sym);

#define _sym_with_index(index) (Obj){ .i = (index << shift_sym) | ot_sym }

static Obj sym_with_index(Int i) {
  check(i < max_sym_index, "Sym index is too large: %lx", i);
  return _sym_with_index(i);
}

static Int sym_index(Obj s) {
  assert(obj_tag(s) == ot_sym);
  return s.i >> shift_sym;
}


// special value constants.
#define DEF_CONSTANT(c) \
static const Obj c = _sym_with_index(si_##c)

DEF_CONSTANT(F); // false
DEF_CONSTANT(T); // true
DEF_CONSTANT(E); // empty
DEF_CONSTANT(N); // none
DEF_CONSTANT(VOID);
DEF_CONSTANT(QUO);
DEF_CONSTANT(QUA);
DEF_CONSTANT(DO);
DEF_CONSTANT(SCOPE);
DEF_CONSTANT(LET);
DEF_CONSTANT(IF);
DEF_CONSTANT(FN);
DEF_CONSTANT(EXPA);


// each Sym object is a small integer indexing into this array of strings.
static Array global_sym_names = array0;


static Obj new_int(Int i) {
  check(i <= max_Int_tagged, "large Int values not yet suppported.");
  return (Obj){ .i = (i << width_tag | ot_int) };
}


static Obj new_uns(Uns u) {
  check(u < max_Uns_tagged, "large Uns values not yet supported.");
  return new_int(cast(Int, u));
}


static Obj new_data(SS s) {
  Obj d = ref_alloc(st_Data_large, size_RCWL + s.len);
  d.rcwl->len = s.len;
  memcpy(data_large_ptr(d).m, s.b.c, s.len);
  return ref_add_tag(d, ot_strong);
}


static Obj new_vec(Mem m) {
  Obj v = ref_alloc(st_Vec_large, size_RCWL + size_Obj * m.len);
  v.rcwl->len = m.len;
  Obj* p = vec_large_ptr(v);
  for_in(i, m.len) {
    p[i] = obj_retain_strong(m.els[i]);
  }
  return ref_add_tag(v, ot_strong);
}


static Obj new_v2(Obj a, Obj b) {
  Mem m = mem_mk((Obj[]){a, b}, 2);
  return new_vec(m);
}


static Obj new_chain(Mem m) {
  Obj c = E;
  for_in_rev(i, m.len) {
    c = new_v2(m.els[i], c);
  }
  return c;
}


static Obj new_sym(SS s) {
  Obj d = new_data(s);
  Int i = array_append(&global_sym_names, d);
  return sym_with_index(i);
}


static void init_syms() {
  assert(global_sym_names.mem.len == 0);
  Obj sym;
  #define A(i, n) sym = new_sym(ss_from_BC(n)); assert(sym_index(sym) == i)
  A(si_F,     "#f");
  A(si_T,     "#t");
  A(si_E,     "#e");
  A(si_N,     "#n");
  A(si_VOID,  "#void");
  A(si_QUO,   "QUO");
  A(si_QUA,   "QUA");
  A(si_DO,    "DO");
  A(si_SCOPE, "SCOPE");
  A(si_LET,   "LET");
  A(si_IF,    "IF");
  A(si_FN,    "FN");
  A(si_EXPA,  "EXPA");
  #undef A
}



