// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "10-int.h"


#define ERR_SYM(s) errFL("SYM %s: u:%lu; si:%li", #s, s.u, sym_index(s));

static const Int shift_sym = width_obj_tag + 1; // 1 bit reserved for Data-word flag.
static const Int sym_index_end = 1L << (size_Int * 8 - shift_sym);

// each Sym object is an index into this array of strings.
struct Array;
static Array global_sym_names = array0;


#define _sym_with_index(index) (Obj){ .u = (cast(Uns, index) << shift_sym) | ot_sym_data }

static Obj sym_with_index(Int i) {
  check(i < sym_index_end, "Sym index is too large: %lx", i);
  return _sym_with_index(i);
}


static Int sym_index(Obj s) {
  assert(obj_is_sym(s));
  return s.i >> shift_sym;
}


// borrow the data for a sym.
// the name is a bit confusing due to overlap with ot_sym_data (the tag for sym or data-word).
static Obj sym_data_borrow(Obj s) {
  assert(obj_is_sym(s));
  return obj_borrow(mem_el(global_sym_names.mem, sym_index(s)));
}


static SS data_SS(Obj d);
static Obj new_data_from_SS(SS s);

static Obj new_sym(SS s) {
  for_in(i, global_sym_names.mem.len) {
    Obj d = obj_borrow(array_el(global_sym_names, i));
    if (ss_eq(s, data_SS(d))) {
      return sym_with_index(i);
    }
  }
  Obj d = new_data_from_SS(s);
  Int i = array_append(&global_sym_names, d);
  Obj sym = sym_with_index(i);
  //errF("NEW SYM: %ld: ", i); obj_errL(sym);
  return sym;
}

// for use with %*s formatter.
#define FMT_SYM(sym) cast(I32, ref_len(sym_data_borrow(sym))), data_ptr(sym_data_borrow(sym))


typedef enum {
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
  si_VOID, // must be last.
} Sym_index;


// special value constants.
#define DEF_SYM(c) \
static const Obj c = _sym_with_index(si_##c)

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
DEF_SYM(VOID);


static void sym_init() {
  assert(global_sym_names.mem.len == 0);
  Obj sym;
#define SBC(bc, si) sym = new_sym(ss_from_BC(bc)); assert(sym_index(sym) == si)
#define S(s) SBC(#s, si_##s)
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
  S(VOID);
#undef SBC
#undef S
}


static Bool sym_is_form(Obj s) {
  assert(obj_is_sym(s));
  return (s.u >= si_COMMENT && s.u <= si_CALL);
}

