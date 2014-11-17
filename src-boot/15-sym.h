// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "14-int.h"


#define ERR_SYM(s) errFL("SYM %s: u:%lu; si:%li", #s, s.u, sym_index(s))

static const Int sym_index_end = 1L << (size_Int * 8 - width_sym_tags);

// each Sym object is an index into this list of strings.
static List global_sym_names;


#define _sym_with_index(index) Obj(Uns((Uns(index) << width_sym_tags) | ot_sym))

static Obj sym_with_index(Int i) {
  check(i < sym_index_end, "Sym index is too large: %lx", i);
  return _sym_with_index(i);
}


Obj Obj::sym_data() {
  assert(is_sym());
  return global_sym_names.el(sym_index());
}


static Obj data_new_from_str(Str s);

static Obj sym_new(Str s) {
  static Hash_map<String, Obj> cache;
  Obj& sym = cache[s]; // on missing key, inserts default, obj0.
  if (!sym.vld()) {
    Obj d = data_new_from_str(s);
    Int i = global_sym_names.append(d);
    sym = sym_with_index(i);
    //errFL("NEW SYM: %ld: %o", i, sym);
  }
  return sym.ret_val();
}


static Obj sym_new_from_chars(Chars c) {
  return sym_new(Str(c));
}


static Obj sym_new_from_c(Chars c) {
  // create a symbol after converting underscores to dashes.
  Int len = chars_len(c);
  CharsM chars = CharsM(raw_alloc(len + 1, ci_Chars));
  memcpy(chars, c, size_t(len + 1));
  for_in(i, len) {
    if (chars[i] == '_') {
      chars[i] = '-';
    }
  }
  Obj sym = sym_new(Str(chars));
  raw_dealloc(chars, ci_Chars);
  return sym;
}


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
#define S(s) const Obj s_##s = _sym_with_index(si_##s);
SYM_LIST
#undef S

#undef SYM_LIST


static void sym_init() {
  assert(global_sym_names.len() == 0);
  for_in(i, si_END) {
    Chars name = sym_index_names[i];
    Obj sym = sym_new_from_c(name);
    assert(sym.sym_index() == i);
    sym.rel_val();
  }
}


Bool Obj::is_special_sym() {
  return sym_index() <= si_END_SPECIAL_SYMS;
}

