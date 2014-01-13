// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "04-ss.h"


typedef U8 Tag;
typedef Uns Sym; // index into global_sym_table.

#define width_obj_tag 4

static const Int width_tagged_word = width_word - width_obj_tag;

static const Uns obj_tag_end = 1L << width_obj_tag;
static const Uns obj_tag_mask = obj_tag_end - 1;
static const Uns obj_body_mask = ~obj_tag_mask;
static const Uns flt_body_mask = max_Uns - 1;
static const Int max_Int_tagged  = (1L  << width_tagged_word) - 1;
static const Uns max_Uns_tagged  = max_Int_tagged;
static const Int shift_factor_Int = 1L << width_obj_tag; // cannot shift signed values in C so use multiplication instead.

/*
all ploy objects are tagged pointer/value words.
the 4-bit obj tag is guaranteed to be available by 16-byte aligned malloc.

Sym/Data-word values: 4 bit (1 nibble) tag, 28/60 bit (7/15 nibble) value.
the next bit is the data-word bit.
if set, the next 3 bits are the length, and the remaining 3/7 bytes are the data bytes.
if not set, the values are indices into the global symbol table.

Ref types:
each ref is tagged as either borrowed, strong, weak, or gc (possible future implementation).
Masking out the low tag bits yields a heap pointer, whose low four bits are the struct tag.
*/

// low bit indicates 32/64 bit IEEE 754 float with low bit rounded to even.
// alternatively we could use two set bits to indicate floats, and fit the enum into a 3 bit tag (discarding the reserved tag values).
static const Tag ot_flt_bit = 1; // only flt words have low bit set.
static const Tag ot_unm_mask = 1 << 3; // unmanaged words have high tag bit set.
static const Tag ot_val_mask = ot_flt_bit | ot_unm_mask; // val words have at least one of these bits set.

typedef enum {
  ot_borrowed   = 0,      // ref is not retained by receiver.
  ot_strong     = 1 << 1, // ref is strongly retained by receiver.
  ot_weak       = 2 << 1, // ref is weakly retained by receiver.
  ot_gc         = 3 << 1, // ref; reserved for possible future garbage collection implementation.
  ot_int        = 4 << 1, // val; 28/60 bit signed int; 28 bits is +/-134M.
  ot_sym_data   = 5 << 1, // val; interleaved 27/59 bit Sym indices and 0-3/7 byte Data-word values.
  ot_reserved0  = 6 << 1, // val.
  ot_reserved1  = 7 << 1, // val.
} Obj_tag;

// the plan is to pack small data objects into words, interleaved with syms.
// for the time being though we will define only the blank string.
static const Uns data_word_bit = (1 << width_obj_tag);


static void assert_obj_tags_are_valid() {
  assert(!(ot_val_mask & ot_strong));
  assert(ot_val_mask & ot_int);
}


// to facilitate direct lookup we duplicate Flt across all possible bit patterns.
static BC obj_tag_names[] = {
  "Borrowed",
  "Flt",
  "Strong",
  "Flt",
  "Weak",
  "Flt",
  "GC",
  "Flt",
  "Int",
  "Flt",
  "Sym",
  "Flt",
  "Reserved0",
  "Flt",
  "Reserved1",
  "Flt",
};


#define width_struct_tag 4
static const Uns struct_tag_end = 1L << width_struct_tag;
static const Uns struct_tag_mask = struct_tag_end - 1;

#define width_meta_tag 4

typedef enum {
  st_Data,
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
  st_DEALLOC = struct_tag_mask, // this value must be equal to the mask for ref_dealloc.
} Struct_tag;

static BC struct_tag_names[] = {
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
  "DEALLOC",
};

static const Int width_sc  = width_word - width_struct_tag;
static const Int width_wc  = width_word - width_meta_tag;
static const Uns pinned_sc = (1L < width_sc) - 1;
static const Uns pinned_wc = (1L < width_wc) - 1;

typedef struct {
  Tag st : width_struct_tag;
  Uns wc : width_wc;
  Tag mt : width_meta_tag;
  Uns sc : width_sc;
} RC; // Ref-counts. 

typedef struct {
  RC rc;
  Int len;
} RCL;

static const Int size_RC  = sizeof(RC);
static const Int size_RCL = sizeof(RCL);


static void rc_errL(BC msg, RC* rc) {
  errF("%s %p {st:%s w:%lx mt:%x s:%lx}\n",
       msg, rc, struct_tag_names[rc->st], rc->wc, rc->mt, rc->sc);
}

#if VERBOSE_MM
#define rc_errLV(...) errL_rc(__VA_ARGS__)
#else
#define rc_errLV(...)
#endif


// tag bits must be masked off before dereferencing all pointer types.
typedef union {
  Int i;
  Uns u;
  Flt f;
  Ptr p;
  RC* rc;
  RCL* rcl;
} Obj;

static Int size_Obj = sizeof(Obj);


static void obj_errL(Obj o);

static NORETURN error_obj(BC msg, Obj o) {
  errF("%s error: %s: ", (process_name ? process_name : __FILE__), msg);
  obj_errL(o);
  exit(1);
}

#define check_obj(condition, msg, o) { if (!(condition)) error_obj(msg, o); }



static Obj_tag obj_tag(Obj o) {
  return o.u & obj_tag_mask;
}


static void obj_errL(Obj o);


static Bool obj_is_val(Obj o) {
  return (o.u & ot_val_mask);
}


static Bool obj_is_flt(Obj o) {
  return (o.u & ot_flt_bit);
}


static Bool obj_is_int(Obj o) {
  return obj_tag(o) == ot_int;
}


static Bool obj_is_num(Obj o) {
  return obj_is_flt(o) || obj_is_int(o);
}


static Bool obj_is_sym(Obj o) {
  return obj_tag(o) == ot_sym_data && !(o.u & data_word_bit);
}


static Bool obj_is_ref(Obj o) {
  return !obj_is_val(o);
}


static Obj obj_ref_borrow(Obj o) {
  assert(obj_is_ref(o));
  return (Obj){ .u = o.u & obj_body_mask };
}


static Obj obj_borrow(Obj o) {
  if (obj_is_ref(o)) {
    return obj_ref_borrow(o);
  }
  else return o;
}


static Bool obj_is_data_word(Obj o) {
  return obj_tag(o) == ot_sym_data && (o.u & data_word_bit);
}


static Bool ref_is_data(Obj o);
static Bool ref_is_vec(Obj o);
static Bool ref_is_file(Obj o);
static Bool ref_is_func_host(Obj o);


static Bool obj_is_data(Obj o) {
  return obj_is_data_word(o) || (obj_is_ref(o) && ref_is_data(obj_ref_borrow(o)));
}


static Bool obj_is_vec(Obj o) {
  return obj_is_ref(o) && ref_is_vec(obj_ref_borrow(o));
}


static Bool obj_is_file(Obj o) {
  return obj_is_ref(o) && ref_is_file(obj_ref_borrow(o));
}


static Bool obj_is_func_host(Obj o) {
  return obj_is_ref(o) && ref_is_func_host(obj_ref_borrow(o));
}


static void assert_obj_is_strong(Obj o) {
  assert(obj_tag(o) == ot_strong || obj_is_val(o));
}


static void assert_ref_is_valid(Obj o);
static Obj ref_add_tag(Obj o, Tag t);

#define IF_OT_IS_VAL_RETURN(...) \
if (ot & ot_val_mask) return __VA_ARGS__ // value; no counting.

static Obj obj_retain_strong(Obj o) {
  Obj_tag ot = obj_tag(o);
  IF_OT_IS_VAL_RETURN(o);
  assert_ref_is_valid(o);
  rc_errLV("ret strong", o.rc);
  if (o.rc->sc < pinned_sc - 1) {
    o.rc->sc++;
  }
  else if (o.rc->sc < pinned_sc) {
    o.rc->sc++;
    errFL("object strong count pinned: %p", o.p);
  }
  return ref_add_tag(o, ot_strong);
}


static Obj obj_retain_weak(Obj o) {
  Obj_tag ot = obj_tag(o);
  IF_OT_IS_VAL_RETURN(o);
  assert_ref_is_valid(o);
  rc_errLV("ret weak ", o.rc);
  if (o.rc->wc < pinned_wc - 1) {
    o.rc->wc++;
  }
  else if (o.rc->wc < pinned_wc) {
    o.rc->wc++;
    errFL("object weak count pinned: %p", o.p);
  }
  return ref_add_tag(o, ot_weak);
}


static void ref_release_strong(Obj r);
static void ref_release_weak(Obj r);

static void obj_release(Obj o) {
  Obj_tag ot = obj_tag(o);
  IF_OT_IS_VAL_RETURN();
  if (ot == ot_borrowed) return; // or error?
  Obj r = obj_borrow(o);
  if (ot == ot_strong) ref_release_strong(r);
  else if (ot == ot_weak) ref_release_weak(r);
  else {
    error("unknown tag: %x", ot);
 }
}


static void write_repr_obj(File f, Obj o);

static void obj_errL(Obj o) {
  write_repr_obj(stderr, o);
  err_nl();
}


static void obj_err_tag(Obj o) {
  BC otn = obj_tag_names[obj_tag(o)];
  err(otn);
}


void dbg(Obj o);
void dbg(Obj o) {
  obj_err_tag(o);
  err(" : ");
  obj_errL(o);
}


