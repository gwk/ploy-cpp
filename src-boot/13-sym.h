// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "12-dict.h"


// notes:
// syms with index lower than END_SPECIAL_SYMS are self-evaluating;
// syms after END_SPECIAL_SYMS are looked up.
// END_SPECIAL_SYMS cannot be evaluated.
// the special forms are COMMENT...CALL.
// the following "X Macro" is expanded with various temporary definitions of SYM.
#define SYM_LIST \
S(DISSOLVED) \
S(void) \
S(nil) \
S(ENV_END) \
S(ENV_FRAME_KEY) \
S(ENV_FRAME_VAL) \
S(UNINIT) \
S(false) \
S(true) \
S(INFER_STRUCT) \
S(INFER_SEQ) \
S(INFER_EXPR) \
S(EXPAND) \
S(RUN) \
S(CONS) \
S(HALT) \
S(END_SPECIAL_SYMS) \
S(self) \
S(a) \
S(b) \
S(c) \
S(callee) \
S(types) \


// sym indices.
#define S(s) si_##s,
enum Sym_index {
  SYM_LIST
  si_END,
};
#undef S

#define S(s) #s,
static Chars sym_index_names[] = {
  SYM_LIST
};
#undef S

// sym Obj constants.
#define S(s) const Obj s_##s = Obj::Sym_with_index(si_##s);
SYM_LIST
#undef S

#undef SYM_LIST


// each Sym object is an index into this list of strings.
static List sym_names;


static void sym_init() {
  assert(sym_names.len() == 0);
  for_in(i, si_END) {
    Chars name = sym_index_names[i];
    Obj sym = Obj::Sym_from_c(name);
    assert(sym.sym_index() == i);
    sym.rel_val();
  }
}


Bool Obj::is_special_sym() const {
  return sym_index() <= si_END_SPECIAL_SYMS;
}


Obj Obj::sym_data() const {
  assert(is_sym());
  return sym_names.el(sym_index());
}


Obj Obj::Sym(Str s) {
  static Hash_map<String, Obj> cache;
  Obj& sym = cache[s]; // on missing key, inserts default, obj0.
  if (!sym.vld()) {
    Obj d = Obj::Data(s);
    Int i = sym_names.append(d);
    sym = Sym_with_index(i);
    //errFL("NEW SYM: %ld: %o", i, sym);
  }
  return sym.ret_val();
}

