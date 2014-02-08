// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "05-str.h"


typedef Uns Tag;
typedef Uns Sym; // index into global_sym_table.

#define width_obj_tag 3
#define obj_tag_end (1L << width_obj_tag)

static const Int width_tagged_word = width_word - width_obj_tag;

static const Uns obj_tag_mask = obj_tag_end - 1;
static const Uns obj_body_mask = ~obj_tag_mask;
static const Uns flt_body_mask = max_Uns - 1;
static const Int max_Int_tagged  = (1L  << width_tagged_word) - 1;
static const Uns max_Uns_tagged  = max_Int_tagged;
static const Int shift_factor_Int = 1L << width_obj_tag; // cannot shift signed values in C so use multiplication instead.


/*
all ploy objects are tagged pointer/value words.
the 3-bit obj tag is guaranteed to be available by 8-byte or greater aligned malloc.
OSX guarantees 16-byte aligned malloc.

Sym values are indices into the global symbol table.

Ref types have a tag of 000 so that they can be dereferenced directly.
*/

// the low tag bit indicates 32/64 bit IEEE 754 float with low bit rounded to even.
static const Tag ot_flt_bit = 1; // only flt words have low bit set.

typedef enum {
  ot_ref  = 0,      // pointer to managed object
  ot_int  = 1 << 1, // val; 28/60 bit signed int; 28 bits is +/-134M.
  ot_sym  = 2 << 1, // Sym index
  ot_data = 3 << 1, // word-sized Data
} Obj_tag;

// note: previously there was a scheme to interleave Sym indices with word-sized Data values.
// this is still possible, but only desirable if we need the last ot enum slot for something else.
static const Uns data_word_bit = (1 << width_obj_tag);
UNUSED_VAR(data_word_bit)

// to facilitate direct lookup we duplicate Flt across all possible bit patterns.
static CharsC obj_tag_names[] = {
  "Ref",
  "Flt",
  "Int",
  "Flt",
  "Sym",
  "Flt",
  "Data-word",
  "Flt",
};


#define width_struct_tag 4
#define struct_tag_end (1L << width_struct_tag)

static const Uns struct_tag_mask = struct_tag_end - 1;

#define width_meta_tag 4

typedef enum {
  st_Data = 0, // obj_counter_index assumes that this is the first index.
  st_Vec,
  st_I32,
  st_I64,
  st_U32,
  st_U64,
  st_F32,
  st_F64,
  st_File,
  st_Func_host,
  st_Reserved_A,
  st_Reserved_B,
  st_Reserved_C,
  st_Reserved_D,
  st_Reserved_E,
  st_Reserved_F,
} Struct_tag;

static CharsC struct_tag_names[] = {
  "Data",
  "Vec",
  "I32",
  "I64",
  "U32",
  "U64",
  "F32",
  "F64",
  "File",
  "Func-host",
  "Reserved-A",
  "Reserved-B",
  "Reserved-C",
  "Reserved-D",
  "Reserved-E",
  "Reserved-F",
};

static const Int width_sc  = width_word - width_struct_tag;
static const Int width_wc  = width_word - width_meta_tag;
static const Uns pinned_sc = (1L << width_sc) - 1;
static const Uns pinned_wc = (1L << width_wc) - 1;

typedef struct {
  Tag st : width_struct_tag;
  Uns wc : width_wc;
  Tag mt : width_meta_tag;
  Uns sc : width_sc;
} RC; // Ref-counts. 

typedef struct {
  RC rc;
  Int len;
} RCL; // Ref-counts, length.

static const Int size_RC  = sizeof(RC);
static const Int size_RCL = sizeof(RCL);

static void rc_err(RC* rc) {
  errF("%p {st:%s w:%lx mt:%x s:%lx}",
       rc, struct_tag_names[rc->st], rc->wc, rc->mt, rc->sc);
}

// TODO: remove?
UNUSED_FN static void rc_errML(CharsC msg, RC* rc) {
  errF("%s %p {st:%s w:%lx mt:%x s:%lx}\n",
       msg, rc, struct_tag_names[rc->st], rc->wc, rc->mt, rc->sc);
}

#if VERBOSE_MM
#define rc_errMLV(...) rc_errML(__VA_ARGS__)
#else
#define rc_errMLV(...)
#endif


typedef union {
  Int i;
  Uns u;
  Flt f;
  Ptr p;
  RC* rc;
  RCL* rcl;
} Obj;

static Int size_Obj = sizeof(Obj);

static const Obj obj0 = (Obj){.p=NULL}; // invalid object.

static void obj_errL(Obj o);

static NORETURN error_obj(CharsC msg, Obj o) {
  errF("%s error: %s: ", (process_name ? process_name : __FILE__), msg);
  obj_errL(o);
  exit(1);
}

#define check_obj(condition, msg, o) { if (!(condition)) error_obj(msg, o); }


static void obj_errL(Obj o); // declared for occasional debugging.


static Obj_tag obj_tag(Obj o) {
  return o.u & obj_tag_mask;
}


static Bool obj_is_val(Obj o) {
  return (o.u & obj_tag_mask);
}


static Bool obj_is_ref(Obj o) {
  return !obj_is_val(o);
}


static Bool obj_is_flt(Obj o) {
  return (o.u & ot_flt_bit);
}


static Bool obj_is_int(Obj o) {
  return obj_tag(o) == ot_int;
}


UNUSED_FN static Bool obj_is_num(Obj o) {
  return obj_is_flt(o) || obj_is_int(o);
}


static Bool obj_is_sym(Obj o) {
  return obj_tag(o) == ot_sym;
}


static Bool sym_is_symbol(Obj o);

// TODO: improve this naming subtlety.
static Bool obj_is_symbol(Obj o) {
  return obj_is_sym(o) && sym_is_symbol(o);
}


static const Obj TRUE, FALSE;

static Bool obj_is_bool(Obj s) {
  return s.u == TRUE.u || s.u == FALSE.u;
}


static Bool obj_is_data_word(Obj o) {
  // note: if we choose to interleave Sym and Data-word, this requires additonal checks.
  return obj_tag(o) == ot_data;
}


static Bool ref_is_data(Obj o);
static Bool ref_is_vec(Obj o);
static Bool ref_is_file(Obj o);


static Bool obj_is_data_ref(Obj o) {
  return obj_is_ref(o) && ref_is_data(o);
}


UNUSED_FN static Bool obj_is_data(Obj o) {
  return obj_is_data_word(o) || obj_is_data_ref(o);
}



static Bool obj_is_vec_ref(Obj o) {
  return obj_is_ref(o) && ref_is_vec(o);
}


static const Obj VEC0, CHAIN0;

static Bool obj_is_vec(Obj o) {
  return o.u == VEC0.u || o.u == CHAIN0.u || obj_is_vec_ref(o);
}


UNUSED_FN static Bool obj_is_file(Obj o) {
  return obj_is_ref(o) && ref_is_file(o);
}


static Int vec_len(Obj v);
static Obj vec_el(Obj v, Int i);
static const Obj LABEL, VARIAD;

static Bool obj_is_par(Obj o) {
  if (!obj_is_vec(o)) return false;
  Int len = vec_len(o);
  if (len != 4) return false;
  if (!obj_is_symbol(vec_el(o, 1))) return false; // name must be a symbol.
  Obj e0 = vec_el(o, 0);
  return (e0.u == LABEL.u || e0.u == VARIAD.u);
}


static Counter_index st_counter_index(Struct_tag st) {
  // note: this math relies on the layout of both COUNTER_LIST and Struct_tag.
  return ci_Data + (st * 2);
}


static Struct_tag ref_struct_tag(Obj r);

static Counter_index obj_counter_index(Obj o) {
  Obj_tag ot = obj_tag(o);
  if (ot & ot_flt_bit) return ci_Flt;
  switch (ot) {
    case ot_int: return ci_Int;
    case ot_sym: return ci_Sym;
    case ot_data: return ci_Data_word;
    case ot_ref: break;
  }
  return st_counter_index(ref_struct_tag(o));
}


static void assert_ref_is_valid(Obj o);

static Obj obj_ret(Obj o) {
  counter_inc(obj_counter_index(o));
  if (obj_tag(o)) return o;
  assert_ref_is_valid(o);
  rc_errMLV("ret strong", o.rc);
  if (o.rc->sc < pinned_sc - 1) {
    o.rc->sc++;
  }
  else if (o.rc->sc < pinned_sc) {
    o.rc->sc++;
    if (report_pinned_counts) {
      errFL("object strong count pinned: %p", o.p);
    }
  } // else previously pinned.
  return o;
}


static void ref_dealloc(Obj o);

static void obj_rel(Obj o) {
  counter_dec(obj_counter_index(o));
  if (obj_tag(o)) return;
  assert_ref_is_valid(o);
  if (o.rc->sc == 1) {
    ref_dealloc(o);
    return;
  }
  rc_errMLV("rel strong", o.rc);
  if (o.rc->sc < pinned_sc) { // can only decrement if the count is not pinned.
    o.rc->sc--;
  }
  rc_errMLV("rel s post", o.rc);
}


static Obj obj_ret_val(Obj o) {
  // ret counting for non-ref objects. a no-op for optimized builds.
  assert(obj_tag(o));
  counter_inc(obj_counter_index(o));
  return o;
}


static Obj obj_rel_val(Obj o) {
  // rel counting for non-ref objects. a no-op for optimized builds.
  assert(obj_tag(o));
  counter_dec(obj_counter_index(o));
  return o;
}


UNUSED_FN static Obj obj_retain_weak(Obj o) {
  if (obj_tag(o)) return o;
  assert_ref_is_valid(o);
  rc_errMLV("ret weak ", o.rc);
  if (o.rc->wc < pinned_wc - 1) {
    o.rc->wc++;
  }
  else if (o.rc->wc < pinned_wc) {
    o.rc->wc++;
    if (report_pinned_counts) {
      errFL("object weak count pinned: %p", o.p);
    }
  } // else previously pinned.
  return o;
}


UNUSED_FN static void obj_release_weak(Obj o) {
  if (obj_tag(o)) return;
  assert_ref_is_valid(o);
  rc_errMLV("rel weak ", o.rc);
  check_obj(o.rc->wc > 0, "over-released weak ref", o);
  // can only decrement if the count is not pinned.
  if (o.rc->wc < pinned_wc) o.rc->wc--;
}


static Int vec_len(Obj v);
static Obj* vec_els(Obj v);

UNUSED_FN static Bool obj_is_quotable(Obj o) {
  // indicates whether an object can be correctly represented inside of a quoted vec.
  // objects whose representation would require explicit quoting to be correct return false,
  // e.g. (File "~/todo.txt")
  // note: the File example feels somewhat contrived, because evaluating that call is questionable
  // (creating new file handle is not desirable, does not recreate the original due to process state).
  // TODO: perhaps non-transparent objects should have an intentionally non-parseable repr?
  if (obj_tag(o)) return true; // all value types are quotable.
  Struct_tag st = ref_struct_tag(o);
  if (st == st_Data) return true; // quotable.
  if (st == st_Vec) {
    Int len = vec_len(o);
    Obj* els = vec_els(o);
    for_in(i, len) {
      if (!obj_is_quotable(els[i])) return false;
    }
    return true;
  }
  return false;
}


static void write_repr(CFile f, Obj o);

static void obj_err(Obj o) {
  write_repr(stderr, o);
}

static void obj_errL(Obj o) {
  write_repr(stderr, o);
  err_nl();
}


static void obj_err_tag(Obj o) {
  CharsC otn = obj_tag_names[obj_tag(o)];
  err(otn);
}


void dbg(Obj o);
void dbg(Obj o) {
  if (obj_tag(o)) {
    obj_err_tag(o);
  }
  else { // ref
    rc_err(o.rc);
  }
  err(" : ");
  obj_errL(o);
  err_flush();
}


