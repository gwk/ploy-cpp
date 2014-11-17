// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// dynamic object type.
// every object word has a tag in the low-order bits indicating its type.
// all objects are either value words with a nonzero tag in the low bits,
// or else references to allocated memory, which have a zero tag
// (the zero tag is guaranteed due to heap allocation alignment).

#include "07-str.h"


// reference count increment functions check for overflow,
// and keep the object's count pinned at the max value.
// this constant controls whether this event gets logged.
static const Bool report_pinned_counts = true;

#define width_obj_tag 2
#define obj_tag_end (1L << width_obj_tag)

static const Int width_tagged_word = width_Word - width_obj_tag;

static const Uns obj_tag_mask = obj_tag_end - 1;
static const Uns obj_body_mask = ~obj_tag_mask;
static const Int max_Int_tagged = (1L << width_tagged_word) - 1;
static const Uns max_Uns_tagged = max_Int_tagged;
// we cannot shift signed values in C so we use multiplication by powers of 2 instead.
static const Int scale_factor_Int = 1L << width_obj_tag;

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
// this implies that Obj_tag can be at most 4 bits wide (4 + 1 + 3 == 8 bits in the low byte).
static const Uns data_word_bit = (1 << width_obj_tag);

static const Int width_sym_tags = width_obj_tag + 1; // extra bit for Data-word flag.

static const Int sym_index_end = 1L << (size_Int * 8 - width_sym_tags);

static Chars obj_tag_names[] = {
  "Ref",
  "Ptr",
  "Int",
  "Sym",
};

struct Head { // common header for all heap objects.
  Raw type; // actually Obj; declared as untyped so that Head can be declared above Obj.
  Uns rc;
  Head(Raw t): type(t), rc((1<<1) + 1) {}
} ALIGNED_TO_WORD;

struct Data {
  Head head;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Data);

struct Cmpd {
  Head head;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Cmpd);

struct Env;
struct Type;

union Obj;

extern const Int size_Obj;
extern const Obj s_true, s_false, s_DISSOLVED;
extern Obj t_Data, t_Env, t_Int, t_Ptr, t_Sym, t_Type;

static Obj env_rel_fields(Obj o);
static Obj type_name(Obj t);

union Obj {
  Int i;
  Uns u;
  Raw r;
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

  #define obj0 Obj()
  #define int0 Obj(Int(ot_int))
  #define blank Obj(Uns(ot_sym | data_word_bit)) // zero length data word.

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

  Bool ref_is_data() const { return ref_type() == t_Data; }

  Bool ref_is_env() const { return ref_type() == t_Env; }

  Bool ref_is_cmpd() const { return !ref_is_data() && !ref_is_env(); }

  Bool ref_is_type() const { return ref_type() == t_Type; }

  Bool is_data_ref() const { return is_ref() && ref_is_data(); }

  Bool is_data() const { return is_data_word() || is_data_ref(); }

  Bool is_env() const { return is_ref() && ref_is_env(); }

  Bool is_cmpd() const { return is_ref() && ref_is_cmpd(); }

  Bool is_type() const { return is_ref() && ref_is_type(); }

  Obj type() const {
    switch (tag()) {
      case ot_ref: return ref_type();
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
  
  struct Hash_is {
    Int operator()(Obj o) const { return o.id_hash(); }
  };

  Counter_index counter_index() const {
    Obj_tag ot = tag();
    switch (ot) {
      case ot_ref: {
        if (ref_is_data()) return ci_Data_ref_rc;
        else if (ref_is_env()) return ci_Env_rc;
        else return ci_Cmpd_rc;
      }
      case ot_ptr: return ci_Ptr_rc;
      case ot_int: return ci_Int_rc;
      case ot_sym: return is_data_word() ? ci_Data_word_rc : ci_Sym_rc;
    }
  }

  // ref
  
  Obj ref_type() const {
    assert(is_ref());
    return Obj(h->type);
  }
  
  // rc
  
  Uns rc() const {
    // get the object's retain count for debugging purposes.
    if (is_val()) return max_Uns;
    assert(h->rc & 1); // TODO: support indirect counts.
    return h->rc >> 1; // shift off the direct flag bit.
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

  Obj ret() const {
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
  
  void rel() const {
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
      counter_dec(o.counter_index());
      assert(o.h->rc & 1); // TODO: support indirect counts.
      if (o.h->rc == (1<<1) + 1) { // count == 1.
        o = o.dealloc(); // returns tail object to be released.
      } else {
        o.h->rc -= 2; // decrement by two to leave the flag bit intact.
        break;
      }
    } while (o.vld());
  }
  
  void dissolve() const {
    if (is_cmpd()) {
      cmpd_dissolve_fields();
    }
    rel();
  }

  Obj dealloc() const {
    assert(is_ref());
    //errFL("DEALLOC: %p:%o", r, *this);
    h->rc = 0;
    ref_type().rel();
    Obj tail;
    if (ref_is_data()) { // no extra action required.
      tail = obj0;
    } else if (ref_is_env()) {
      tail = env_rel_fields(*this);
    } else {
      tail = cmpd_rel_fields();
    }
    // ret/rel counter has already been decremented by rc_rel.
#if !OPTION_DEALLOC_PRESERVE
    raw_dealloc(r, Counter_index(counter_index() + 1));
#elif OPTION_ALLOC_COUNT
    counter_dec(Counter_index(counter_index() + 1)); // do not dealloc, just count.
#endif
    return tail;
  }
  
  // Ptr
  
  Raw ptr() const {
    assert(is_ptr());
    return Raw(u & ~obj_tag_mask);
  }

  static Obj with_Ptr(Raw p) {
    Uns u = Uns(p);
    assert(!(u & obj_tag_mask));
    return Obj(Uns(u | ot_ptr)).ret_val();
  }
  
  // Int
  
  Int int_val() const {
    assert(tag() == ot_int);
    Int val = i & Int(obj_body_mask);
    return val / scale_factor_Int;
  }

  static Obj with_Int(Int i) {
    check(i >= -max_Int_tagged && i <= max_Int_tagged, "large Int values not yet suppported.");
    Int shifted = i * scale_factor_Int;
    return Obj(Int(shifted | ot_int)).ret_val();
  }
  
  
  static Obj with_U64(U64 u) {
    check(u < U64(max_Uns_tagged), "large Uns values not yet supported.");
    return with_Int(Int(u));
  }
  
  // Sym
  
  Int sym_index() const {
    assert(is_sym());
    return i >> width_sym_tags;
  }
  
  static Obj Sym_with_index(Int idx) {
    check(idx < sym_index_end, "Sym index is too large: %lx", idx);
    return Obj(Uns((Uns(idx) << width_sym_tags) | ot_sym));
  }

  Bool is_special_sym() const;
  
  Obj sym_data() const;
  
  static Obj Sym(Str s);

  static Obj Sym(Chars c) {
    return Sym(Str(c));
  }

  static Obj Sym_from_c(Chars c) {
    // create a symbol after converting underscores to dashes.
    Int len = chars_len(c);
    CharsM chars = CharsM(raw_alloc(len + 1, ci_Chars));
    memcpy(chars, c, size_t(len + 1));
    for_in(i, len) {
      if (chars[i] == '_') {
        chars[i] = '-';
      }
    }
    Obj sym = Sym(chars);
    raw_dealloc(chars, ci_Chars);
    return sym;
  }


  // Bool
  
  Bool is_true_bool() const {
    if (*this == s_true) return true;
    if (*this == s_false) return false;
    assert(0);
    exit(1);
  }

  Bool is_true() const {
    switch (tag()) {
      case ot_ref: {
        Obj type = ref_type();
        if (type == t_Data) return *this != blank;
        if (type == t_Env) return true;
        return !!cmpd_len();
      }
      case ot_ptr:
        return (ptr() != null);
      case ot_int:
        return *this != int0;
      case ot_sym:
        return (u >= s_true.u);
    }
    assert(0);
  }
  
  static Obj with_Bool(Bool b) {
    return (b ? s_true : s_false).ret_val();
  }
  
  // Data
  
  Int data_ref_len() const {
    assert(ref_is_data());
    assert(d->len > 0);
    return d->len;
  }
  
  Int data_len() const {
    if (*this == blank) return 0; // TODO: support all data-word values.
    return data_ref_len();
  }
  
  Chars data_ref_chars() const {
    assert(ref_is_data());
    return reinterpret_cast<Chars>(d + 1); // address past data header.
  }
  
  CharsM data_ref_charsM() const {
    assert(ref_is_data());
    return reinterpret_cast<CharsM>(d + 1); // address past data header.
  }
  
  Chars data_chars() const {
    if (*this == blank) return null; // TODO: support all data-word values.
    return data_ref_chars();
  }
  
  Str data_str() const {
    return Str(data_len(), data_chars());
  }
  
  Bool data_ref_iso(Obj o) const {
    Int len = data_ref_len();
    return len == o.data_ref_len() && !memcmp(data_ref_chars(), o.data_ref_chars(), Uns(len));
  }
  
  static Obj data_new_raw(Int len) {
    counter_inc(ci_Data_ref_rc);
    Obj o = Obj(raw_alloc(size_Data + len, ci_Data_ref_alloc));
    *o.h = Head(t_Data.ret().r);
    o.d->len = len;
    return o;
  }
  
  static Obj Data(Str s) {
    if (!s.len) return blank.ret_val();
    Obj d = data_new_raw(s.len);
    memcpy(d.data_ref_charsM(), s.chars, Uns(s.len));
    return d;
  }
  
  static Obj Data(Chars c) {
    Uns len = strnlen(c, max_Int);
    check(len <= max_Int, "%s: string exceeded max length", __func__);
    Obj d = data_new_raw(Int(len));
    memcpy(d.data_ref_charsM(), c, len);
    return d;
  }
  
  static Obj Data_from_path(Chars path) {
    CFile f = fopen(path, "r");
    check(f, "could not open file: %s", path);
    fseek(f, 0, SEEK_END);
    Int len = ftell(f);
    if (!len) return blank.ret_val();
    Obj d = data_new_raw(len);
    fseek(f, 0, SEEK_SET);
    Uns items_read = fread(d.data_ref_charsM(), size_Char, Uns(len), f);
    check(Int(items_read) == len,
          "read failed; expected len: %i; actual: %u; path: %s", len, items_read, path);
    fclose(f);
    return d;
  }
  
  // Cmpd.

  Int cmpd_len() const {
    assert(ref_is_cmpd());
    assert(c->len >= 0);
    return c->len;
  }

  Obj* cmpd_els() const {
    assert(ref_is_cmpd());
    return reinterpret_cast<Obj*>(c + 1); // address past header.
  }

  Obj cmpd_el(Int idx) const {
    assert(ref_is_cmpd());
    assert(idx >= 0 && idx < cmpd_len());
    return cmpd_els()[idx];
  }

  Obj cmpd_el_move(Int idx) const {
    assert(ref_is_cmpd());
    assert(idx >= 0 && idx < cmpd_len());
    Obj* slot = cmpd_els() + idx;
    Obj el = *slot;
#if OPTION_MEM_ZERO
    *slot = obj0;
#endif
    return el;
  }

  void cmpd_put(Int idx, Obj el) const {
    assert(ref_is_cmpd());
    assert(idx >= 0 && idx < cmpd_len());
    Obj* slot = cmpd_els() + idx;
#if OPTION_MEM_ZERO
    assert(!slot->vld());
#endif
    *slot = el;
  }

  Range<Obj*>cmpd_it() const {
    Obj* b = cmpd_els();
    return Range<Obj*>(b, b + cmpd_len());
  }

  Range<Obj*>cmpd_to(Int to) const {
    assert(to <= cmpd_len());
    Obj* b = cmpd_els();
    return Range<Obj*>(b, b + to);
  }

  Obj cmpd_slice(Int fr, Int to) const {
    assert(ref_is_cmpd());
    Int l = cmpd_len();
    if (fr < 0) fr += l;
    if (to < 0) to += l;
    fr = int_clamp(fr, 0, l);
    to = int_clamp(to, 0, l);
    Int ls = to - fr; // length of slice.
    if (ls == l) {
      return *this; // no ret/rel necessary.
    }
    Obj slice = Obj::Cmpd_raw(ref_type().ret(), ls);
    Obj* src = cmpd_els();
    Obj* dst = slice.cmpd_els();
    for_in(idx, ls) {
      dst[idx] = src[idx + fr].ret();
    }
    return slice;
  }

  Obj cmpd_rel_fields() const {
    Int len = cmpd_len();
    if (!len) return obj0; // return the termination sentinel for rc_rel tail loop.
    Int last_i = len - 1;
    for_mut(el, cmpd_to(last_i)) {
      el.rel();
    }
#if OPTION_TCO
    return cmpd_el(last_i);
#else
    cmpd_el(last_i).rel();
    return obj0;
  #endif
  }

  void cmpd_dissolve_fields() const {
    for_mut(el, cmpd_it()) {
      el.rel();
      el = s_DISSOLVED.ret_val();
    }
  }

  static Obj Cmpd_raw(Obj type, Int len) {
    // owns type.
    counter_inc(ci_Cmpd_rc);
    Obj o = Obj(raw_alloc(size_Cmpd + (size_Obj * len), ci_Cmpd_alloc));
    *o.h = Head(type.r);
    o.c->len = len;
  #if OPTION_MEM_ZERO
    memset(o.cmpd_els(), 0, Uns(size_Obj * len));
  #endif
    return o;
  }
  
  static Obj _Cmpd(Obj type, Int i, Obj el) {
    // owns all arguments.
    Obj o = Cmpd_raw(type, i + 1);
    o.cmpd_put(i, el);
    return o;
  }

  template <typename T, typename... Ts>
  static Obj _Cmpd(Obj type, Int i, T el, Ts... rest) {
    // owns all arguments.
    Obj o = _Cmpd(type, i + 1, rest...);
    o.cmpd_put(i, el);
    return o;
  }

  template <typename T, typename... Ts>
  static Obj Cmpd(Obj type, T el, Ts... rest) {
    // owns all arguments.
    return _Cmpd(type, 0, el, rest...);
  }

};
DEF_SIZE(Obj);
