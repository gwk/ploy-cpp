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

enum Obj_tag {
  ot_ref = 0,  // pointer to managed object.
  ot_ptr = 1,  // Host pointer.
  ot_int = 2,  // val; 30/62 bit signed int; 30 bits gives a range of  +/-536M.
  ot_sym = 3,  // Sym values are indices into global_sym_table, interleaved with Data words.
};

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

static const Int width_sym_tags = width_obj_tag + 1; // extra bit for Data-word flag.

static Chars obj_tag_names[] = {
  "Ref",
  "Ptr",
  "Int",
  "Sym",
};

struct Ref_head;
struct Data;
struct Env;
struct Cmpd;
struct Type;

extern const Obj s_true, s_false;
extern Obj t_Ptr, t_Int, t_Data, t_Sym;

static Bool ref_is_data(Obj o);
static Bool ref_is_env(Obj o);
static Bool ref_is_cmpd(Obj o);
static Bool ref_is_type(Obj o);
static Obj ref_type(Obj r);

union Obj {
  Int i;
  Uns u;
  Raw r;
  Chars chars; // no valid object can be cast to chars; this for the exc_raise formatter.
  Ref_head* h; // common to all ref types.
  Data* d;
  Env* e;
  Cmpd* c;
  Type* t;

  Obj(): r(null) {} // constructs the invalid object; essentially the null pointer.
  // this works because references have the zero tag.
  
  explicit Obj(Int _i): i(_i) {} // TODO: change semantics to shift?
  explicit Obj(Uns _u): u(_u) {} // TODO: change semantics to shift?
  explicit Obj(Raw _r): r(_r) {}
  explicit Obj(Type* _t): t(_t) {}

  Bool operator==(Obj o) { return u == o.u; }
  Bool operator!=(Obj o) { return u != o.u; }

  Bool vld() { return *this != Obj(); }
   
  Obj_tag tag() {
    assert(vld());
    return cast(Obj_tag, u & obj_tag_mask);
  }

  Bool is_val() {
    assert(vld());
    return tag() != ot_ref;
  }

  Bool is_ref() {
    assert(vld());
    return tag() == ot_ref;
  }

  Bool is_ptr() {
    assert(vld());
    return tag() == ot_ptr;
  }

  Bool is_int() {
    assert(vld());
    return tag() == ot_int;
  }

  Bool is_sym() {
    assert(vld());
    return tag() == ot_sym;
  }

  Bool is_bool() {
    assert(vld());
    return *this == s_true || *this == s_false;
  }

  Bool is_data_word() {
    return tag() == ot_sym && u & data_word_bit;
  }

  Bool is_data_ref() {
    return is_ref() && ref_is_data(*this);
  }

  Bool is_data() {
    return is_data_word() || is_data_ref();
  }

  Bool is_env() {
    return is_ref() && ref_is_env(*this);
  }

  Bool is_cmpd() {
    return is_ref() && ref_is_cmpd(*this);
  }

  Bool is_type() {
    return is_ref() && ref_is_type(*this);
  }

  Obj type() {
    switch (tag()) {
      case ot_ref: return ref_type(*this);
      case ot_ptr: return t_Ptr;
      case ot_int: return t_Int;
      case ot_sym: return (is_data_word() ? t_Data : t_Sym);
    }
  }

  Int id_hash() {
    switch (tag()) {
      case ot_ref: return cast(Int, u >> width_min_alloc);
      case ot_ptr: return cast(Int, u >> width_min_alloc);
      case ot_int: return cast(Int, u >> width_obj_tag);
      case ot_sym: return cast(Int, u >> width_sym_tags);
    }
  }

#if OPTION_ALLOC_COUNT
  Counter_index counter_index() {
    Obj_tag ot = tag();
    switch (ot) {
      case ot_ref: return ci_Ref_rc;
      case ot_ptr: return ci_Ptr_rc;
      case ot_int: return ci_Int_rc;
      case ot_sym: return is_data_word() ? ci_Data_word_rc : ci_Sym_rc;
    }
  }
#endif
};
DEF_SIZE(Obj);

#define obj0 Obj()
