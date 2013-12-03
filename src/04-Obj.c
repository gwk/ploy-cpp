// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "03-SS.c"


typedef U8 Tag;
typedef Uns Sym; // index into global_sym_table.

#define width_tag 4

#if ARCH_32_WORD
typedef U16 UH; // unsigned half-word.
#define width_UH 16
#define width_tagged_word 28
#define width_tagged_UH 12

#elif ARCH_64_WORD
typedef U32 UH; // unsigned half-word.
#define width_UH 32
#define width_tagged_word 60
#define width_tagged_UH 28

#endif

static const Int size_UH = sizeof(UH);

// tagged pointer value mask.
static const Uns tag_end = 1L << width_tag;
static const Uns tag_mask = tag_end - 1;
static const Uns val_mask = ~tag_mask;
static const Int max_Int_tagged  = (1L  << width_tagged_word) - 1;
static const Uns max_Uns_tagged  = max_Int_tagged;

/*
all ploy objects are tagged pointer/value words.
the 4-bit obj tag is guaranteed to be available by 16-byte aligned malloc.
there is also a 4-bit tag carved out of each of the ref-count half-words.
these are called the struct tag and spec tag.

Sym/Data-word values: 4 bit (1 nibble) tag, 28/60 bit (7/15 nibble) value.
the next bit is the data-word bit.
if set, the next 3 bits are the length, and the remaining 3/7 bytes are the data bytes.
if not set, the values are indices into the global symbol table.

Ref types:
each ref is tagged as either borrowed, strong, weak, or gc (possible future implementation).
Masking out the low tag bits yields a heap pointer, whose low four bits are the struct tag.
*/

typedef enum {
  ot_managed   = 0,      // 0000; available for use in hosted system on top of ploy.
  ot_flt_bit   = 1,      // xxx1; (float/double with low fraction bit rounded to even).
  ot_reserved  = 1 << 1, // 0010
  ot_int       = 2 << 1, // 0100; (28/60 bit signed int); 28 bits is +/-134M.
  ot_sym       = 3 << 1, // 0110; interleaved Sym indices and Data-word values.
  ot_borrowed  = 4 << 1, // 1000; ref is not retained by receiver.
  ot_strong    = 5 << 1, // 1010; ref is strongly retained by receiver.
  ot_weak      = 6 << 1, // 1100; ref is weakly reteined by receiver.
  ot_gc        = 7 << 1, // 1110; reserved for possible future garbace collection implementation.
} Obj_tag;

static const Tag ot_ref_bit = 1 << 3;

typedef enum {
  st_Data_large,
  st_Vec_large,
  st_I32,
  st_I64,
  st_U32,
  st_U64,
  st_F32,
  st_F64,
  st_Proxy = 0xF,
} Struct_tag;


// Struct_tag bits must be the first four bits. This is not currently big-endian compatible.
typedef struct {
  UH w; // struct tag | weak count.
  UH s; // spec_tag | strong count.
} ALIGNED_TO_WORD RCH; // Reference count, half-words.

typedef struct {
  Uns w; // struct tag | weak count.
  Uns s; // spec_tag | strong count.
} RCW; // Reference count, full-words.

typedef struct {
  RCW rc;
  Int len;
} RCWL;

static const Int size_RCH  = sizeof(RCH);
static const Int size_RCW  = sizeof(RCW);
static const Int size_RCWL = sizeof(RCWL);


// ref counts are unshifted high bits;
// counting is done in count_inc increments to obviate tag masking.
static const UH count_inc = tag_end;
static const UH count_mask_h = (1L << width_UH) - tag_end; // also the max count value.
static const Uns count_mask_w = max_Uns - tag_mask; // also the max count value.


static Tag rch_struct_tag(RCH rch) { return rch.w & tag_mask; }
static Tag rcw_struct_tag(RCW rcw) { return rcw.w & tag_mask; }
static Tag rch_spec_tag(RCH rch)   { return rch.s & tag_mask; }
static Tag rcw_spec_tag(RCW rcw)   { return rcw.s & tag_mask; }
static UH rch_weak(RCH rch)        { return rch.w & count_mask_h; }
static Uns rcw_weak(RCW rcw)       { return rcw.w & count_mask_w; }
static UH rch_strong(RCH rch)      { return rch.s & count_mask_h; }
static Uns rcw_strong(RCW rcw)     { return rcw.s & count_mask_w; }

static const Bool report_pinned_counts = true;

static Bool ch_is_pinned(UH c)   { return c >= count_mask_h; }
static Bool cw_is_pinned(Uns c)  { return c >= count_mask_w; }

#define DEF_RC_INC(RC, s, f) \
static void rc##s##_##inc##_##f(RC* rc) { \
  if (c##s##_is_pinned(rc->f)) return; \
  rc->f += count_inc; \
  if (report_pinned_counts && c##s##_is_pinned(rc->f)) { \
    errL("object " #RC " " #s " count pinned: %p", rc); \
  } \
}

#define DEF_RC_DEC(RC, s, f) \
static void rc##s##_##dec##_##f(RC* rc) { \
  if (c##s##_is_pinned(rc->f)) return; \
  assert(rc->f >= count_inc); \
  rc->f -= count_inc; \
}

DEF_RC_INC(RCH, h, w)
DEF_RC_INC(RCH, h, s)
DEF_RC_INC(RCW, w, w)
DEF_RC_INC(RCW, w, s)

DEF_RC_DEC(RCH, h, w)
DEF_RC_DEC(RCH, h, s)
DEF_RC_DEC(RCW, w, w)
DEF_RC_DEC(RCW, w, s)


// tag bits must be masked off before dereferencing all pointer types.
typedef union {
  Int i;
  Uns u;
  Ptr p;
  Struct_tag* st;
  //RCH* rch;
  RCW* rcw;
  RCWL* rcwl;
} Obj;

static Int size_Obj = sizeof(Obj);


static Obj_tag obj_tag(Obj o) {
  return o.u & tag_mask;
}

#define assert_ref_masked(r) assert(obj_tag(r) == 0)

static Obj obj_strip_tag(Obj o) {
  return (Obj){ .u = o.u & val_mask };
}

static Obj ref_add_tag(Obj r, Tag t) {
  assert_ref_masked(r);
  return (Obj){ .u = r.u | t };
}

static Obj obj_replace_tag(Obj o, Tag t) {
  return ref_add_tag(obj_strip_tag(o), t);
}


static Struct_tag ref_struct_tag(Obj r) {
  assert_ref_masked(r);
  return *r.st & tag_mask;
}


static Int ref_large_len(Obj r) {
  assert_ref_masked(r);
  assert(ref_struct_tag(r) == st_Data_large || ref_struct_tag(r) == st_Vec_large);
  return r.rcwl->len;
}


static B data_large_ptr(Obj r) {
  assert_ref_masked(r);
  assert(ref_struct_tag(r) == st_Data_large);
  return (B){.m = cast(BM, r.rcwl + 1)};
}


static Obj* vec_large_ptr(Obj r) {
  assert_ref_masked(r);
  assert(ref_struct_tag(r) == st_Vec_large);
  return cast(Obj*, r.rcwl + 1);
}


#define RETURN_IF_OT_VAL(...) \
if (ot & ot_flt_bit || ot < ot_ref_bit) return __VA_ARGS__ // value; no counting.


static Obj obj_retain_weak(Obj o) {
  Obj_tag ot = obj_tag(o);
  RETURN_IF_OT_VAL(o);
  assert(ot == ot_borrowed);
  Obj r = obj_strip_tag(o);
  rcw_inc_w(r.rcw);
  return ref_add_tag(r, ot_weak);
}


static Obj obj_retain_strong(Obj o) {
  Obj_tag ot = obj_tag(o);
  RETURN_IF_OT_VAL(o);
  if (ot == ot_strong) return o; // already owned.
  assert(ot == ot_borrowed);
  Obj r = obj_strip_tag(o);
  rcw_inc_s(r.rcw);
  return ref_add_tag(r, ot_strong);
}


static void ref_release_weak(Obj r) {
  assert_ref_masked(r);
  rcw_dec_w(r.rcw);
}


static void ref_dealloc(Obj r);

static void ref_release_strong(Obj r) {
  assert_ref_masked(r);
  if (r.rcw->s < count_inc * 2) { // ref count is 1.
    ref_dealloc(r);
  }
  else {
    rcw_dec_s(r.rcw);
  }
}


static void obj_release(Obj o) {
  Obj_tag ot = obj_tag(o);
  RETURN_IF_OT_VAL();
  Obj r = obj_strip_tag(o);
  if (ot == ot_weak)        ref_release_weak(r);
  else if (ot == ot_strong) ref_release_strong(r);
  else {
   error("unknown tag: %x", ot);
 }
}


static void ref_dealloc(Obj r) {
  assert_ref_masked(r);
  check(r.rcw->w < count_inc, "attempted to deallocate object with non-zero weak count: %p", r.p);
  Struct_tag st = ref_struct_tag(r);
  if (st == st_Vec_large) {
    Obj* els = vec_large_ptr(r);
    for_in(i, ref_large_len(r)) {
      obj_release(els[i]); // TODO: make this tail recursive for deallocating long chains?
    }
  }
  free(r.p);
}


// return Obj must be tagged ot_strong or else dealloced directly.
static Obj ref_alloc(Struct_tag st, Int width) {
  Obj r = (Obj){.p=malloc((Uns)width)};
  r.rcw->w = st;
  r.rcw->s = count_inc;
  return r;
}


