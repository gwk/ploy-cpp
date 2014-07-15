// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-int.h"


#define ERR_SYM(s) errFL("SYM %s: u:%lu; si:%li", #s, s.u, sym_index(s));

static const Int shift_sym = width_obj_tag + 1; // extra bit for Data-word flag.
static const Int sym_index_end = 1L << (size_Int * 8 - shift_sym);

// each Sym object is an index into this array of strings.
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
static Obj data_new_from_str(Str s);

static Obj sym_new(Str s) {
  for_in(i, global_sym_names.mem.len) {
    Obj d = mem_el(global_sym_names.mem, i);
    if (str_eq(s, data_str(d))) {
      return rc_ret_val(sym_with_index(i));
    }
  }
  Obj d = data_new_from_str(s);
  Int i = array_append(&global_sym_names, d);
  Obj sym = sym_with_index(i);
  //errFL("NEW SYM: %ld: %o", i, sym);
  return rc_ret_val(sym);
}


static Obj sym_new_from_chars(Chars b) {
  return sym_new(str_from_chars(b));
}


// for use with "%.*s" formatter.
#define FMT_SYM(sym) FMT_STR(data_str(sym_data(sym)))

// notes:
// ILLEGAL is a special value for errors inside of well-formed data structures;
// in contrast, obj0 is never present inside of valid data structures.
// syms with index lower than END_SPECIAL_SYMS are self-evaluating;
// syms after END_SPECIAL_SYMS are looked up.
// END_SPECIAL_SYMS cannot be evaluated.
// the special forms are COMMENT...CALL.
// the following "X Macro" is expanded with various temporary definitions of SYM.
#define SYM_LIST \
S(ILLEGAL) \
S(void) \
S(nil) \
S(ENV_END_MARKER) \
S(ENV_FRAME_MARKER) \
S(UNINIT) \
S(false) \
S(true) \
S(INFER_STRUCT) \
S(INFER_SEQ) \
S(INFER_PAR) \
S(INFER_EXPR) \
S(Ptr) \
S(Int) \
S(Sym) \
S(Data) \
S(Env) \
S(Unq) \
S(Qua) \
S(Expand) \
S(Comment) \
S(RUNNABLE_BEGIN) \
S(Eval) \
S(Quo) \
S(Do) \
S(Scope) \
S(Let) \
S(If) \
S(Fn) \
S(Syn_struct_typed) \
S(Syn_seq_typed) \
S(Call) \
S(RUNNABLE_END) \
S(Obj) \
S(Label) \
S(Variad) \
S(Par) \
S(Func) \
S(File) \
S(Src) \
S(Expr) \
S(Syn_struct) \
S(Syn_seq) \
S(Mem_Obj) \
S(Mem_Par) \
S(Mem_Expr) \
S(Mem_Int) \
S(END_SPECIAL_SYMS) \
S(self) \
S(a) \
S(b) \
S(c) \

// sym indices.
#define S(s) si_##s,
typedef enum {
  SYM_LIST
} Sym_index;
#undef S

#define S(s) #s,
static Chars_const sym_index_names[] = {
  SYM_LIST
};
#undef S

// sym Obj constants.
#define S(s) static const Obj s_##s = _sym_with_index(si_##s);
SYM_LIST
#undef S

#undef SYM_LIST


static Ref_tag ref_tag(Obj r) {
  assert(obj_is_ref(r));
  Int si = sym_index(*r.type_ptr);
  if (si == si_Data) return rt_Data;
  if (si == si_Env) return rt_Env;
  return rt_Struct;
}


static void sym_init() {
  assert(global_sym_names.mem.len == 0);
  for_in(i, si_self + 1) { // NOTE: self is not special, but it is the last hardcoded sym.
    Chars_const name = sym_index_names[i];
    // special name cases.
    if (i == si_Syn_struct) name = "Syn-struct";
    else if (i == si_Syn_struct_typed) name = "Syn-struct-typed";
    else if (i == si_Syn_seq) name = "Syn-seq";
    else if (i == si_Syn_seq_typed) name = "Syn-seq-typed";
    else if (i == si_Mem_Obj) name = "Mem-Obj";
    else if (i == si_Mem_Par) name = "Mem-Par";
    else if (i == si_Mem_Expr) name = "Mem-Expr";
    else if (i == si_Mem_Int) name = "Mem-Int";
    Obj sym = sym_new_from_chars(cast(Chars, name));
    assert(sym_index(sym) == i);
    rc_rel_val(sym);
  }
}


UNUSED_FN static Bool sym_is_form(Obj s) {
  Int si = sym_index(s);
  return si >= si_Comment && si <= si_Fn;
}


static Bool sym_is_special(Obj s) {
  return sym_index(s) <= si_END_SPECIAL_SYMS;
}