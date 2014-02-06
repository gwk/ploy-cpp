// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "12-int.h"


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
#define FMT_SYM(sym) cast(I32, data_len(sym_data(sym))), data_ptr(sym_data(sym))

// notes:
// ILLEGAL is a special value for returning during error conditions; completely prohibited in ploy code.
// syms with index lower than VOID are self-evaluating; syms after VOID are looked up.
// VOID cannot be evaluated, but is a legal return value (which must be ignored).
// the special forms are COMMENT...FN.
// the following "X Macro" is expanded with various temporary definitions of SYM.
#define SYM_LIST \
SYM(ILLEGAL) \
SYM(NIL) \
SYM(VEC0) \
SYM(CHAIN0) \
SYM(END) \
SYM(FALSE) \
SYM(TRUE) \
SYM(LABEL) \
SYM(VARIAD) \
SYM(UNQ) \
SYM(QUA) \
SYM(EXPA) \
SYM(COMMENT) \
SYM(QUO) \
SYM(DO) \
SYM(SCOPE) \
SYM(LET) \
SYM(IF) \
SYM(FN) \
SYM(VOID) \
SYM(Vec) \


// sym indices.
#define SYM(s) si_##s,
typedef enum {
  SYM_LIST
} Sym_index;
#undef SYM

// sym Obj constants.
#define SYM(s) static const Obj s = _sym_with_index(si_##s);
SYM_LIST
#undef SYM


static void sym_init() {
  assert(global_sym_names.mem.len == 0);
  Obj sym;

#define SYM(s) sym = new_sym(ss_from_BC(#s)); assert(sym_index(sym) == si_##s); obj_rel_val(sym);
SYM_LIST
#undef SYM
}


static bool sym_is_form(Obj s) {
  Int si = sym_index(s);
  return si >= si_COMMENT && si <= si_FN;
}


static bool sym_is_symbol(Obj s) {
  // this behavior matches that of run_sym.
  return sym_index(s) > si_VOID;
}

