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
UNUSED static const Uns ot_flt_bit = 1; // only flt words have low bit set.
UNUSED static const Uns flt_body_mask = max_Uns - 1;

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

struct Head;
struct Data;
struct Env;
struct Cmpd;
struct Type;

extern const Obj s_true, s_false;
extern Obj t_Ptr, t_Int, t_Data, t_Sym;

static Obj ref_dealloc(Obj o);
static Bool ref_is_cmpd(Obj o);
static Bool ref_is_data(Obj o);
static Bool ref_is_env(Obj o);
static Bool ref_is_type(Obj o);
static Obj ref_type(Obj r);

static void cmpd_dissolve_fields(Obj c);

union Obj {
  Int i;
  Uns u;
  Raw r;
  Chars chars; // no valid object can be cast to chars; this for the exc_raise formatter.
  Head* h; // common to all ref types.
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

  Bool operator==(const Obj o) const { return u == o.u; }
  Bool operator!=(const Obj o) const { return u != o.u; }

  Bool vld() const { return *this != Obj(); }
   
  Obj_tag tag() const {
    assert(vld());
    return Obj_tag(u & obj_tag_mask);
  }

  Bool is_val() const {
    assert(vld());
    return tag() != ot_ref;
  }

  Bool is_ref() const {
    assert(vld());
    return tag() == ot_ref;
  }

  Bool is_ptr() const {
    assert(vld());
    return tag() == ot_ptr;
  }

  Bool is_int() const {
    assert(vld());
    return tag() == ot_int;
  }

  Bool is_sym() const {
    assert(vld());
    return tag() == ot_sym;
  }

  Bool is_bool() const {
    assert(vld());
    return *this == s_true || *this == s_false;
  }

  Bool is_data_word() const {
    return tag() == ot_sym && u & data_word_bit;
  }

  Bool is_data_ref() const {
    return is_ref() && ref_is_data(*this);
  }

  Bool is_data() const {
    return is_data_word() || is_data_ref();
  }

  Bool is_env() const {
    return is_ref() && ref_is_env(*this);
  }

  Bool is_cmpd() const {
    return is_ref() && ref_is_cmpd(*this);
  }

  Bool is_type() const {
    return is_ref() && ref_is_type(*this);
  }

  Obj type() const {
    switch (tag()) {
      case ot_ref: return ref_type(*this);
      case ot_ptr: return t_Ptr;
      case ot_int: return t_Int;
      case ot_sym: return (is_data_word() ? t_Data : t_Sym);
    }
  }

  Int id_hash() const {
    switch (tag()) {
      case ot_ref: return Int(u >> width_min_alloc);
      case ot_ptr: return Int(u >> width_min_alloc);
      case ot_int: return Int(u >> width_obj_tag);
      case ot_sym: return Int(u >> width_sym_tags);
    }
  }

  void dissolve() {
    if (is_cmpd()) {
      cmpd_dissolve_fields(*this);
    }
    rel();
  }

  Obj ret_val() const {
    // ret counting for non-ref objects. a no-op for optimized builds.
    assert(is_val());
    counter_inc(counter_index());
    return *this;
  }

  Obj rel_val() const {
    // rel counting for non-ref objects. a no-op for optimized builds.
    assert(is_val());
    counter_dec(counter_index());
    return *this;
  }

  Counter_index counter_index() const {
    Obj_tag ot = tag();
    switch (ot) {
      case ot_ref: return ci_Ref_rc;
      case ot_ptr: return ci_Ptr_rc;
      case ot_int: return ci_Int_rc;
      case ot_sym: return is_data_word() ? ci_Data_word_rc : ci_Sym_rc;
    }
  }

  Uns rc() const;
  Obj ret();
  void rel();

};
DEF_SIZE(Obj);

#define obj0 Obj()


struct Head {
  Obj type;
  Uns rc;
  Head(Obj t): type(t), rc(0) {}
};


Uns Obj::rc() const {
  // get the object's retain count for debugging purposes.
  if (is_val()) return max_Uns;
  assert(h->rc & 1); // TODO: support indirect counts.
  return h->rc >> 1; // shift off the direct flag bit.
}


Obj Obj::ret() {
  // increase the object's retain count by one.
  assert(vld());
  counter_inc(counter_index());
  if (is_val()) return *this;
  check(h->rc, "object was prematurely deallocated: %p", r);
  assert(h->rc & 1); // TODO: support indirect counts.
  assert(h->rc < max_Uns);
  h->rc += 2; // increment by two to leave the flag bit intact.
  return *this;
}

void Obj::rel() {
  // decrease the object's retain count by one, or deallocate it.
  Obj o = *this;
  assert(vld());
  do {
    if (o.is_val()) {
      counter_dec(o.counter_index());
      return;
    }
    if (!o.h->rc) {
      // cycle deallocation is complete, and we have arrived at an already-deallocated member.
      errFL("CYCLE: %p", o.r);
      assert(0); // TODO: support cycles.
      return;     
    }
    counter_dec(counter_index());
    assert(o.h->rc & 1); // TODO: support indirect counts.
    if (o.h->rc == (1<<1) + 1) { // count == 1.
      o = ref_dealloc(o); // returns tail object to be released.
    } else {
      o.h->rc -= 2; // decrement by two to leave the flag bit intact.
      break;
    }
  } while (o.vld());
}


