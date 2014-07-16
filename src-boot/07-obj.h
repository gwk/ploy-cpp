// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// dynamic object type.
// every object word has a tag in the low-order bits indicating its type.
// all objects are either value words with a nonzero tag in the low bits,
// or else references to allocated memory, which have a zero tag
// (the zero tag is guaranteed due to heap allocation alignment).

#include "06-str.h"


// reference count increment functions check for overflow,
// and keep the object's count pinned at the max value.
// this constant controls whether this event gets logged.
static const Bool report_pinned_counts = true;

#define width_obj_tag 2
#define obj_tag_end (1L << width_obj_tag)

static const Int width_tagged_word = width_word - width_obj_tag;

static const Uns obj_tag_mask = obj_tag_end - 1;
static const Uns obj_body_mask = ~obj_tag_mask;
static const Int max_Int_tagged  = (1L  << width_tagged_word) - 1;
static const Uns max_Uns_tagged  = max_Int_tagged;
// we cannot shift signed values in C so we use multiplication by powers of 2 instead.
static const Int shift_factor_Int = 1L << width_obj_tag;

typedef enum {
  ot_ref = 0,  // pointer to managed object.
  ot_ptr = 1,  // Host pointer.
  ot_int = 2,  // val; 30/62 bit signed int; 30 bits gives a range of  +/-536M.
  ot_sym = 3,  // Sym values are indices into global_sym_table, interleaved with Data words.
} Obj_tag;

// note: previously there was a scheme to have a 31/63 bit float type;
// the low tag bit indicated 32/64 bit IEEE 754 float with low bit rounded to even.
// the remaining Obj_tag bits were each shifted left by one,
// and the width of Int and Sym were reduced by one bit.
// it remains to be seen just how bad an idea this is for 32-bit applications;
// it also remains to be seen if/how to do the rounding correctly!
static const Uns ot_flt_bit = 1; // only flt words have low bit set.
UNUSED_VAR(ot_flt_bit)
static const Uns flt_body_mask = max_Uns - 1;
UNUSED_VAR(flt_body_mask)

// Sym indices are interleaved with word-sized Data values.
// The low bit after the tag differentiates between Sym (0) and Data (1).
// Data-words would use the remaining bits in the byte to represent the length.
// For 64 bit archs, this length field must count up to 7, so it requires 3 bits;
// this implies that Obj_tag must be at most 4 bits wide (4 + 1 + 3 == 8 bits in the low byte).
static const Uns data_word_bit = (1 << width_obj_tag);

static Chars_const obj_tag_names[] = {
  "Ref",
  "Ptr",
  "Int",
  "Sym",
};

typedef struct _Data Data;
typedef struct _Env Env;
typedef struct _Struct Struct;
typedef struct _Type Type;

typedef union _Obj {
  Int i;
  Uns u;
  Raw r;
  Chars c; // no valid object can be interpreted as Chars; this for the exc_raise formatter.
  union _Obj* type_ptr; // common to all ref types.
  Data* d;
  Env* e;
  Struct* s;
  Type* t;
} Obj;
DEF_SIZE(Obj);


// invalid object; essentially the NULL pointer.
// used as a return value for error conditions and a marker for cleared or invalid memory.
static const Obj obj0 = (Obj){.u=0};


// struct representing a step of the interpreted computation.
// in the normal case, it contains the environment and value resulting from the previous step.
// in the tail-call optimization (TCO) case, it contains the tail step to perform.
typedef struct {
  Obj env; // the env to be passed to the next step, after any TCO steps; the new caller env.
  Obj val; // the result from the previous step, or the env to be passed to the TCO step.
  Obj tco_code; // code for the TCO step.
} Step;

#define mk_step(e, v) (Step){.env=(e), .val=(v), .tco_code=obj0}


static Bool is(Obj a, Obj b) {
  return a.u == b.u;
}


static Obj_tag obj_tag(Obj o) {
  return o.u & obj_tag_mask;
}


UNUSED_FN static Bool obj_is_val(Obj o) {
  return (o.u & obj_tag_mask);
}


static Bool obj_is_ref(Obj o) {
  return obj_tag(o) == ot_ref;
}


static Bool obj_is_ptr(Obj o) {
  return obj_tag(o) == ot_ptr;
}


static Bool obj_is_int(Obj o) {
  return obj_tag(o) == ot_int;
}


static Bool obj_is_sym(Obj o) {
  return obj_tag(o) == ot_sym;
}


static const Obj s_true, s_false;

static Bool obj_is_bool(Obj s) {
  return is(s, s_true) || is(s, s_false);
}


static Bool obj_is_data_word(Obj o) {
  return obj_tag(o) == ot_sym && o.u & data_word_bit;
}


static Bool ref_is_data(Obj o);
static Bool ref_is_env(Obj o);
static Bool ref_is_struct(Obj o);


static Bool obj_is_data_ref(Obj o) {
  return obj_is_ref(o) && ref_is_data(o);
}


static Bool obj_is_data(Obj o) {
  return obj_is_data_word(o) || obj_is_data_ref(o);
}


static Bool obj_is_env(Obj o) {
  return obj_is_ref(o) && ref_is_env(o);
}


static Bool obj_is_struct(Obj o) {
  return obj_is_ref(o) && ref_is_struct(o);
}


static Obj ref_type(Obj r);
static const Obj s_Ptr, s_Int, s_Data, s_Sym;

static Obj obj_type(Obj o) {
  switch (obj_tag(o)) {
    case ot_ref: return ref_type(o);
    case ot_ptr: return s_Ptr;
    case ot_int: return s_Int;
    case ot_sym: return (obj_is_data_word(o) ? s_Data : s_Sym);
  }
}


#if OPT_ALLOC_COUNT

static Counter_index obj_counter_index(Obj o) {
  Obj_tag ot = obj_tag(o);
  switch (ot) {
    case ot_ref: return ci_Ref_rc;
    case ot_ptr: return ci_Ptr_rc;
    case ot_int: return ci_Int_rc;
    case ot_sym: return obj_is_data_word(o) ? ci_Data_word_rc : ci_Sym_rc;
  }
}
#endif

