// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "04-ss.h"


typedef unsigned Tag;
typedef Uns Sym; // index into global_sym_table.

#define width_obj_tag 3

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
static BC obj_tag_names[] = {
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
  st_Data,
  st_Vec,
  st_I32,
  st_I64,
  st_U32,
  st_U64,
  st_F32,
  st_F64,
  st_File,
  st_Func_host_1,
  st_Func_host_2,
  st_Func_host_3,
  st_Reserved_A,
  st_Reserved_B,
  st_Reserved_C,
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
  "Func-host-1",
  "Func-host-2",
  "Func-host-3",
  "Reserved-A",
  "Reserved-B",
  "Reserved-C",
  "DEALLOC",
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
} RCL;

static const Int size_RC  = sizeof(RC);
static const Int size_RCL = sizeof(RCL);

static void rc_err(RC* rc) {
  errF("%p {st:%s w:%lx mt:%x s:%lx}",
       rc, struct_tag_names[rc->st], rc->wc, rc->mt, rc->sc);
}

static void rc_errML(BC msg, RC* rc) {
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


static void obj_errL(Obj o);

static NORETURN error_obj(BC msg, Obj o) {
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


static Bool obj_is_num(Obj o) {
  return obj_is_flt(o) || obj_is_int(o);
}


static Bool obj_is_sym(Obj o) {
  return obj_tag(o) == ot_sym;
}


static Bool obj_is_data_word(Obj o) {
  // note: if we choose to interleave Sym and Data-word, this requires additonal checks.
  return obj_tag(o) == ot_data;
}


static Bool ref_is_data(Obj o);
static Bool ref_is_vec(Obj o);
static Bool ref_is_file(Obj o);


static Bool obj_is_ref_data(Obj o) {
  return obj_is_ref(o) && ref_is_data(o);
}


static Bool obj_is_ref_vec(Obj o) {
  return obj_is_ref(o) && ref_is_vec(o);
}


static Bool obj_is_data(Obj o) {
  return obj_is_data_word(o) || obj_is_ref_data(o);
}


static const Obj VEC0, CHAIN0;

static Bool obj_is_vec(Obj o) {
  // TODO: clarify convention for whether vec_ functions refer to refs only, or include CHAIN0 and VEC0.
  return o.u == VEC0.u || o.u == CHAIN0.u || obj_is_ref_vec(o);
}


static Bool obj_is_file(Obj o) {
  return obj_is_ref(o) && ref_is_file(o);
}


static void assert_ref_is_valid(Obj o);

static Obj obj_retain_strong(Obj o) {
  Obj_tag ot = obj_tag(o);
  if (ot) return o;
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
  }
  return o;
}


static Obj obj_retain_weak(Obj o) {
  Obj_tag ot = obj_tag(o);
  if (ot) return o;
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
  }
  return o;
}


static void ref_dealloc(Obj o);

static void obj_release_strong(Obj o) {
  Obj_tag ot = obj_tag(o);
  if (ot) return;
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

              
static void obj_release_weak(Obj o) {
  Obj_tag ot = obj_tag(o);
  if (ot) return;
  assert_ref_is_valid(o);
  rc_errMLV("rel weak ", o.rc);
  check_obj(o.rc->wc > 0, "over-released weak ref", o);
  // can only decrement if the count is not pinned.
  if (o.rc->wc < pinned_wc) o.rc->wc--;
}


static void write_repr_obj(File f, Obj o);

static void obj_err(Obj o) {
  write_repr_obj(stderr, o);
}

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


