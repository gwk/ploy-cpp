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
  ot_ref  = 0,  // pointer to managed object.
  ot_int  = 1,  // val; 30/62 bit signed int; 30 bits gives a range of  +/-536M.
  ot_sym  = 2,  // Sym values are indices into global_sym_table.
  ot_data = 3,  // small Data object that fits into a word.
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

// note: previously there was a scheme to interleave Sym indices with word-sized Data values.
// this would work by making the next-lowest bit a flag to differentiate between Sym and Data.
// Data-words would use the remaining bits in the byte to represent the length.
// currently that would mean the low byte layout is TTTFLLLL;
// 4 bits is a wide enouh len field for a maximum of 3/7 data bytes.
// this is only desirable if we need the last Obj_tag index slot for something else.
static const Uns data_word_bit = (1 << width_obj_tag);
UNUSED_VAR(data_word_bit)

static Chars_const obj_tag_names[] = {
  "Ref",
  "Int",
  "Sym",
  "Data-word",
};

// all ref types (objects that are dynamically allocated) follow a similar convention;
// the lowest 4 bits of the first allocated word indicate its type.
// NOTE: the current plan is that eventually this will be replaced by a 'type' pointer.
#define width_ref_tag 2
#define ref_tag_end (1L << width_ref_tag)

static const Uns ref_tag_mask = ref_tag_end - 1;

// all the ref types.
typedef enum {
  rt_Data = 0,    // binary data; obj_counter_index assumes that this is the first index.
  rt_Vec,         // a fixed length vector of objects.
  rt_File,        // wrapper around CFile type, plus additional info.
  rt_Func_host,   // an opaque function built into the host interpreter.
} Ref_tag;

static Chars_const ref_tag_names[] = {
  "Data",
  "Vec",
  "File",
  "Func-host",
};

union _Obj;
typedef struct _Vec Vec;
typedef struct _Data Data;
typedef struct _File File;
typedef struct _Func_host Func_host;

typedef union _Obj {
  Int i;
  Uns u;
  Ptr p;
  Chars c; // no valid object can be interpreted as Chars; this for the exc_raise formatter.
  union _Obj* type_ptr; // common to all ref types.
  Vec* vec;
  Data* data;
  File* file;
  Func_host* func_host;
} Obj;
DEF_SIZE(Obj);


// invalid object; essentially the NULL pointer.
// used as a return value for error conditions and a marker for cleared or invalid memory.
static const Obj obj0 = (Obj){.p=NULL};


// struct representing a step of the interpreted computation.
// in the normal case, it contains the environment and value resulting from the previous step.
// in the tail-call optimization (TCO) case, it contains the tail step to perform.
typedef struct {
  Obj env; // the env to be passed to the next step, after any TCO steps; the new caller env.
  Obj val; // the result from the previous step, or the env to be passed to the TCO step.
  Obj tco_code; // code for the TCO step.
} Step;

#define mk_step(e, v) (Step){.env=(e), .val=(v), .tco_code=obj0}


static Obj_tag obj_tag(Obj o) {
  return o.u & obj_tag_mask;
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
  Chars_const otn = obj_tag_names[obj_tag(o)];
  err(otn);
}


void dbg(Obj o); // not declared static so that it is always available in debugger.
void dbg(Obj o) {
  if (obj_tag(o)) {
    obj_err_tag(o);
  } else { // ref.
    errF("%p", o.p);
  }
  err(" : ");
  obj_errL(o);
  err_flush();
}


static NO_RETURN error_obj(Chars_const msg, Obj o) {
  errF("ploy error: %s: ", msg);
  obj_errL(o);
  exit(1);
}

#define check_obj(condition, msg, o) { if (!(condition)) error_obj(msg, o); }


static Bool obj_is_val(Obj o) {
  return (o.u & obj_tag_mask);
}


static Bool obj_is_ref(Obj o) {
  return !obj_is_val(o);
}


static Bool obj_is_int(Obj o) {
  return obj_tag(o) == ot_int;
}


static Bool obj_is_sym(Obj o) {
  return obj_tag(o) == ot_sym;
}


static Bool sym_is_special(Obj o);

// TODO: improve this naming subtlety.
// there are semantic differences between the special predefined Sym values
// and regular syms which, when evaluated, get looked up in the environment.
static Bool obj_is_special(Obj o) {
  return obj_is_sym(o) && sym_is_special(o);
}


static const Obj s_true, s_false;

static Bool obj_is_bool(Obj s) {
  return s.u == s_true.u || s.u == s_false.u;
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


static Bool obj_is_data(Obj o) {
  return obj_is_data_word(o) || obj_is_data_ref(o);
}


static Bool obj_is_vec_ref(Obj o) {
  return obj_is_ref(o) && ref_is_vec(o);
}


static const Obj s_VEC0, s_CHAIN0;

static Bool obj_is_vec(Obj o) {
  // ploy makes distinctions between the zero vector VEC0, the empty list CHAIN0,
  // and the list terminator END to reduce ambiguity (e.g. when printing data structures).
  // only VEC0 and nonzero ref_vec objects are considered true vectors.
  return o.u == s_VEC0.u || obj_is_vec_ref(o);
}


static Bool obj_is_file(Obj o) {
  return obj_is_ref(o) && ref_is_file(o);
}

static Int vec_ref_len(Obj v);
static Int vec_len(Obj v);
static Obj vec_ref_el(Obj v, Int i);
static Obj* vec_ref_els(Obj v);
static Obj* vec_els(Obj v);
static const Obj s_LABEL, s_VARIAD;


// iterate over a vec_ref vr.
#define it_vec_ref(it, vr) \
for (Obj *it = vec_ref_els(vr), *_end_##it = it + vec_ref_len(vr); \
it < _end_##it; \
it++)

// iterate over a vec v by first checking that it is not VEC0.
#define it_vec(it, v) \
if (v.u != s_VEC0.u) it_vec_ref(it, v)


static Bool obj_is_par(Obj o) {
  // a parameter is a vector with first element of LABEL or VARIAD,
  // representing those two syntactic constructs respectively.
  if (!obj_is_vec_ref(o)) return false;
  Int len = vec_ref_len(o);
  if (len != 4) return false;
  if (obj_is_special(vec_ref_el(o, 1))) return false; // name must be a regular sym.
  Obj e0 = vec_ref_el(o, 0);
  return (e0.u == s_LABEL.u || e0.u == s_VARIAD.u);
}


#if OPT_ALLOC_COUNT
static Counter_index ref_counter_index(Obj r);

static Counter_index obj_counter_index(Obj o) {
  Obj_tag ot = obj_tag(o);
  switch (ot) {
    case ot_int: return ci_Int;
    case ot_sym: return ci_Sym;
    case ot_data: return ci_Data_word;
    case ot_ref: return ref_counter_index(o);
  }
}
#endif

