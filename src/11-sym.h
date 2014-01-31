// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "10-int.h"


#define ERR_SYM(s) errFL("SYM %s: u:%lu; si:%li", #s, s.u, sym_index(s));

static const Int shift_sym = width_obj_tag + 1; // 1 bit reserved for Data-word flag.
static const Int sym_index_end = 1L << (size_Int * 8 - shift_sym);

// each Sym object is an index into this array of strings.
struct Array;
static Array global_sym_names = array0;


#define _sym_with_index(index) (Obj){ .u = (cast(Uns, index) << shift_sym) | ot_sym }

static Obj sym_with_index(Int i) {
  check(i < sym_index_end, "Sym index is too large: %lx", i);
  return _sym_with_index(i);
}


static Int sym_index(Obj s) {
  assert(obj_is_sym(s));
  return s.i >> shift_sym;
}


static Obj sym_data(Obj s) {
  assert(obj_is_sym(s));
  return mem_el_borrowed(global_sym_names.mem, sym_index(s));
}


static SS data_SS(Obj d);
static Obj new_data_from_SS(SS s);

static Obj new_sym(SS s) {
  for_in(i, global_sym_names.mem.len) {
    Obj d = mem_el_borrowed(global_sym_names.mem, i);
    if (ss_eq(s, data_SS(d))) {
      return obj_ret_val(sym_with_index(i));
    }
  }
  Obj d = new_data_from_SS(s);
  Int i = array_append_move(&global_sym_names, d);
  Obj sym = sym_with_index(i);
  //errF("NEW SYM: %ld: ", i); obj_errL(sym);
  return obj_ret_val(sym);
}

// for use with "%*s" formatter.
#define FMT_SYM(sym) cast(I32, ref_len(sym_data(sym))), data_ptr(sym_data(sym))


typedef enum {
  si_ILLEGAL, // special value for returning during error conditions; completely prohibited in code.
  si_NIL,
  si_VEC0,
  si_CHAIN0,
  si_END,
  si_FALSE,
  si_TRUE,
  si_COMMENT,
  si_UNQ,
  si_QUA,
  si_QUO,
  si_DO,
  si_SCOPE,
  si_LET,
  si_IF,
  si_FN,
  si_CALL,
  si_EXPA,
  si_LABEL,
  si_VARIAD,
  si_VOID,  // all syms below VOID are self-evaluating; everything above is normal. VOID cannot be evaluated, but is otherwise legal.
} Sym_index;


// special value constants.
#define DEF_SYM(c) \
static const Obj c = _sym_with_index(si_##c)

DEF_SYM(ILLEGAL);
DEF_SYM(NIL);
DEF_SYM(VEC0);
DEF_SYM(CHAIN0);
DEF_SYM(END);
DEF_SYM(FALSE);
DEF_SYM(TRUE);
DEF_SYM(COMMENT);
DEF_SYM(UNQ);
DEF_SYM(QUA);
DEF_SYM(QUO);
DEF_SYM(DO);
DEF_SYM(SCOPE);
DEF_SYM(LET);
DEF_SYM(IF);
DEF_SYM(FN);
DEF_SYM(CALL);
DEF_SYM(EXPA);
DEF_SYM(LABEL);
DEF_SYM(VARIAD);
DEF_SYM(VOID);


static void sym_init() {
  assert(global_sym_names.mem.len == 0);
  Obj sym;
#define SBC(bc, si) sym = new_sym(ss_from_BC(bc)); assert(sym_index(sym) == si); obj_rel_val(sym);
#define S(s) SBC(#s, si_##s)
  S(ILLEGAL);
  S(NIL);
  S(VEC0);
  S(CHAIN0);
  S(END);
  SBC("false", si_FALSE);
  SBC("true", si_TRUE);
  S(COMMENT);
  S(UNQ);
  S(QUA);
  S(QUO);
  S(DO);
  S(SCOPE);
  S(LET);
  S(IF);
  S(FN);
  S(CALL);
  S(EXPA);
  S(LABEL);
  S(VARIAD);
  S(VOID);
#undef SBC
#undef S
}


static Bool sym_is_form(Obj s) {
  assert(obj_is_sym(s));
  return (s.u >= si_COMMENT && s.u <= si_CALL);
}
