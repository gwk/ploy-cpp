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
  return mem_el(global_sym_names.mem, sym_index(s));
}


static Str data_str(Obj d);
static Obj new_data_from_str(Str s);

static Obj new_sym(Str s) {
  for_in(i, global_sym_names.mem.len) {
    Obj d = mem_el(global_sym_names.mem, i);
    if (str_eq(s, data_str(d))) {
      return obj_ret_val(sym_with_index(i));
    }
  }
  Obj d = new_data_from_str(s);
  Int i = array_append(&global_sym_names, d);
  Obj sym = sym_with_index(i);
  //errF("NEW SYM: %ld: ", i); obj_errL(sym);
  return obj_ret_val(sym);
}


static Obj new_sym_from_chars(Chars b) {
  return new_sym(str_from_chars(b));
}


// for use with "%*s" formatter.
#define FMT_SYM(sym) cast(I32, data_len(sym_data(sym))), data_ptr(sym_data(sym))

// notes:
// ILLEGAL is a special value for errors inside of well-formed data structures;
// in contrast, obj0 is never present inside of valid data structures.
// syms with index lower than VOID are self-evaluating; syms after VOID are looked up.
// VOID cannot be evaluated, but is a legal return value (which must be ignored).
// the special forms are COMMENT...CALL.
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
SYM(EXPAND) \
SYM(COMMENT) \
SYM(QUO) \
SYM(DO) \
SYM(SCOPE) \
SYM(LET) \
SYM(IF) \
SYM(FN) \
SYM(SEQ) \
SYM(CALL) \
SYM(VOID) \


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

#define SYM(s) \
sym = new_sym_from_chars(cast(Chars, #s)); \
assert(sym_index(sym) == si_##s); \
obj_rel_val(sym);

SYM_LIST
  
#undef SYM
}


UNUSED_FN static Bool sym_is_form(Obj s) {
  Int si = sym_index(s);
  return si >= si_COMMENT && si <= si_FN;
}


static Bool sym_is_symbol(Obj s) {
  // returns true if s is a normal symbol; when evaluated, it is looked up in the current env.
  // otherwise it is either a predefined constant that is self-evaluating,
  // VOID, which does not evaluate, or ILLEGAL, which should never be present.
  // see run_sym for details.
  return sym_index(s) > si_VOID;
}

