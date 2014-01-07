// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.


// exclude the standard libraries when preprocessing the source for review (see preprocess.sh).
#ifndef SKIP_LIB_INCLUDES
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <Block.h>
#endif


#ifndef OPT_DEALLOC_MARK
#define OPT_DEALLOC_MARK 1
#endif

#ifndef OPT_ALLOC_COUNT
#define OPT_ALLOC_COUNT 1
#endif

#ifndef OPT_MEM_CLEAR_ELS
#define OPT_MEM_CLEAR_ELS 1
#endif

#define VERBOSE_PARSE 0
#define VERBOSE_EVAL 0


typedef char Char;
typedef unsigned char Byte;
typedef long Int;
typedef unsigned long Uns;
typedef bool Bool;

typedef int8_t I8;
typedef uint8_t U8;

typedef int16_t I16;
typedef uint16_t U16;

typedef int32_t I32;
typedef uint32_t U32;

typedef int64_t I64;
typedef uint64_t U64;

typedef float F32;
typedef double F64;

typedef void* Ptr;

typedef union {
  I32 i;
  U32 u;
  F32 f;
} W32;

typedef union {
  I64 i;
  U64 u;
  F64 f;
} W64;

typedef Char* BM; // Bytes-mutable.
typedef const Char* BC; // Bytes-constant.

typedef wchar_t* Utf32M;
typedef const wchar_t* Utf32C;

typedef FILE* File;


#if __SIZEOF_POINTER__ == 4
#define ARCH_32_WORD 1
#define ARCH_64_WORD 0
typedef float Flt;

#elif __SIZEOF_POINTER__ == 8
#define ARCH_32_WORD 0
#define ARCH_64_WORD 1
typedef double Flt;

#else
// could also define ARCH_32_WIDE, which would use long long values.
// this would require additional conditionals.
#error "unknown architecture"
#endif

// generic word.
typedef union {
  Int i;
  Uns u;
  Flt f;
  Ptr p;
} Word;

// integer range constants.
static const Int min_Int = LONG_MIN;
static const Int max_Int = LONG_MAX;
static const Uns max_Uns = ULONG_MAX;

// byte size constants.
static const Int size_Char  = sizeof(Char);
static const Int size_Bool  = sizeof(Bool);
static const Int size_Int   = sizeof(Int);
static const Int size_Uns   = sizeof(Uns);
static const Int size_Flt   = sizeof(Flt);
static const Int size_Ptr   = sizeof(Ptr);
static const Int size_Word  = sizeof(Word);


static void assert_host_basic() {
  assert(size_Word == size_Int);
  assert(size_Word == size_Uns);
  assert(size_Word == size_Flt);
  assert(size_Word == size_Ptr);
  assert(sizeof(wchar_t) == 4);
}


#define len_buffer 256 // for various temporary arrays on the stack.

#define loop while (1)
#define for_imns(i, m, n, s) \
for (Int i = (m), _##i##_end = (n), _##i##_step = (s); i < _##i##_end; i += _##i##_step)

#define for_imns_rev(i, m, n, s) \
for (Int i = (n) - 1, _##i##_end = (m), _##i##_step = (s); i >= _##i##_end; i -= _##i##_step)

#define for_imn(i, m, n)      for_imns(i, (m), (n), 1)
#define for_imn_rev(i, m, n)  for_imns_rev(i, (m), (n), 1)

#define for_in(i, n)      for_imns(i, 0, (n), 1)
#define for_in_rev(i, n)  for_imns_rev(i, 0, (n), 1)


#define sign(x) ({__typeof__(x) __x = (x); __x > 0 ? 1 : (__x < 0 ? -1 : 0); })

// use the cast macro to make all casts easily searchable for audits.
#define cast(t, ...) (t)(__VA_ARGS__)

// used to create switch statements that return strings for enum names.
#define CASE_RET_TOK(t) case t: return #t
#define CASE_RET_TOK_SPLIT(prefix, t) case prefix##t: return #t

// iteration macros

// boolean logic
#define bit(x) ((x) ? true : false)
#define xor(a, b) (bit(a) ^ bit(b))

// mark function as never returning. ex: NORETURN f() {...}
#define NORETURN __attribute__((noreturn)) void

// suppress compiler warnings. ex: UNUSED_FN f() {...}
#define UNUSED_FN __attribute__((unused))

// suppress unused var warnings.
#define STRING_FROM_TOKEN(x) #x
#define UNUSED_VAR(x) _Pragma(STRING_FROM_TOKEN(unused(x)))

// force struct alignment. ex: typedef struct { I32 a, b; } ALIGNED_8 S;
#define ALIGNED_TO_4 __attribute__((__aligned__(4)))
#define ALIGNED_TO_8 __attribute__((__aligned__(8)))

// mark a function as having no side effects.
#define PURE __attribute__((pure))

// const is like pure, but also does not access any values except the arguments;
// dereferencing pointer arguments is disallowed.
#define PURE_VAL __attribute__((const))

// clang c function overloading extension.
#define OVERLOAD __attribute__((overloadable))

#if   ARCH_32_WORD
#define ALIGNED_TO_WORD ALIGNED_TO_4
#elif ARCH_64_WORD
#define ALIGNED_TO_WORD ALIGNED_TO_8
#endif


static Word word_with_Int(Int i) { return (Word){.i=i}; }
static Word word_with_Uns(Uns u) { return (Word){.u=u}; }
static Word word_with_Flt(Flt f) { return (Word){.f=f}; }
static Word word_with_Ptr(Ptr p) { return (Word){.p=p}; }


static Int int_min(Int a, Int b) {
  return (a < b) ? a : b;
}

static Int int_max(Int a, Int b) {
  return (a > b) ? a : b;
}

static Int int_clamp(Int x, Int a, Int b) {
  return int_max(int_min(x, b), a);
}


// inspired by http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static Int round_up_to_power_2(Int x) {
  assert(size_Int >= 4);
  Int r = x - 1;
  r |= r >> 1;
  r |= r >> 2;
  r |= r >> 4;
  r |= r >> 8;
  r |= x >> 16;
  if (size_Int == 8) {
    r |= x >> 32;
  }
  return r + 1;
}


#pragma mark - B


// rather than cast between const and mutable bytes, use this union.
typedef union {
  BC c;
  BM m;
} B;


static Bool bc_eq(BC a, BC b) {
  return strcmp(a, b) == 0;
}


// get the base name of the path argument.
static BC path_base(BC path) {
  Int offset = 0;
  Int i = 0;
  loop {
    if (!path[i]) break; // EOS
    if (path[i] == '/') offset = i + 1;
    i += 1;
  }

  return path + offset; 
}


// the name of the current process.
static BC process_name;

// call in main to set process_name.
static void set_process_name(BC arg0) {
  process_name = path_base(arg0);
}


// stdio utilities.

static void out(BC s) { fputs(s, stdout); }
static void err(BC s) { fputs(s, stderr); }

static void out_nl() { out("\n"); }
static void err_nl() { err("\n"); }

static void out_flush() { fflush(stdout); }
static void err_flush() { fflush(stderr); }

static void outL(BC s) { out(s); out_nl(); }
static void errL(BC s) { err(s); err_nl(); }

#define outF(fmt, ...) fprintf(stdout, fmt, ## __VA_ARGS__)
#define errF(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)

#define outFL(fmt, ...) outF(fmt "\n", ## __VA_ARGS__)
#define errFL(fmt, ...) errF(fmt "\n", ## __VA_ARGS__)

// errD prints if vol_err > 0.
static Int vol_err;

#ifdef NDEBUG
#define errD(...);
#define errLD(...);
#define errFD(...);
#define errFLD(...);
#else
#define errD(...)   { if (vol_err) err(__VA_ARGS__);  }
#define errLD(...)  { if (vol_err) errL(__VA_ARGS__); }
#define errFD(...)  { if (vol_err) errF(__VA_ARGS__); }
#define errFLD(...) { if (vol_err) errFL(__VA_ARGS__); }
#endif

// error macros

#define warn(fmt, ...) errL("warning: " fmt, ## __VA_ARGS__)

#define error(fmt, ...) { \
  errFL("%s error: " fmt, (process_name ? process_name : __FILE__), ## __VA_ARGS__); \
  exit(1); \
}

#define check(condition, fmt, ...) { if (!(condition)) error(fmt, ## __VA_ARGS__); }


#pragma mark - raw


#if OPT_ALLOC_COUNT
static Int total_allocs_raw = 0;
static Int total_allocs_mem = 0;
static Int total_allocs_ref = 0;
static Int total_deallocs_raw = 0;
static Int total_deallocs_mem = 0;
static Int total_deallocs_ref = 0;
#endif

static Ptr raw_alloc(Int size) {
  assert(size >= 0);
#if OPT_ALLOC_COUNT
  if (size > 0) {
    total_allocs_raw++;
  }
#endif
  return malloc(cast(Uns, size));
}


static void raw_dealloc(Ptr p) {
#if OPT_ALLOC_COUNT
  if (p) {
    total_deallocs_raw++;
  }
#endif
  free(p);
}


#pragma mark - SS


// substring type.
typedef struct _SS {
  B b;
  Int len;
} SS;

#define ss0 (SS){.b=(B){.c=NULL}, .len=0}


static SS ss_mk(B b, Int len) {
  return (SS){.b=b, .len=len};
}


static SS ss_mk_c(BC c, Int len) {
  return ss_mk((B){.c=c}, len);
}


static SS ss_mk_m(BM m, Int len) {
  return ss_mk((B){.m=m}, len);
}


static void ss_dealloc(SS s) {
  raw_dealloc(s.b.m);
}


static SS ss_alloc(Int len) {
  // add null terminator for easier debugging.
  BM m = raw_alloc(len + 1);
  m[len] = 0;
  return ss_mk_m(m, len);
}


static void ss_realloc(SS* s, Int len) {
  s->b.m = realloc(s->b.m, cast(Uns, len));
  s->len = len;
}


// return B must be freed.
static B ss_copy_to_B(SS ss) {
  B b = (B){.m=raw_alloc(ss.len + 1)};
  b.m[ss.len] = 0;
  memcpy(b.m, ss.b.c, ss.len);
  return b;
}


static SS ss_from_BC(BC p) {
  Uns len = strlen(p);
  check(len <= max_Int, "string exceeded max length");
  return ss_mk_c(p, cast(Int, len));
}


// create a string from a pointer range.
static SS ss_from_range(BM start, BM end) {
  assert(end <= start);
  return ss_mk_m(start, end - start);
}


static Bool ss_is_valid(SS s) {
  return  (!s.b.c && !s.len) || (s.b.c && s.len > 0);
}


static Bool ss_index_is_valid(SS s, Int i) {
  return i >= 0 && i < s.len;
}


static void check_ss_index(SS s, Int i) {
  assert(ss_is_valid(s));
  check(ss_index_is_valid(s, i), "invalid SS index: %ld", i);
}


static Bool ss_eq(SS a, SS b) {
  return a.len == b.len && memcmp(a.b.c, b.b.c, cast(Uns, a.len)) == 0;
}


static char ss_el(SS s, Int index) {
  check_ss_index(s, index);
  return s.b.c[index];
}


// pointer to end of string
static BC ss_end(SS s) {
  return s.b.c + s.len;
}


static Bool ss_ends_with_char(SS s, Char c) {
  return (s.len > 0 && s.b.c[s.len - 1] == c);
}


// create a substring offset by from.
static SS ss_from(SS s, Int from) {
  assert(from >= 0);
  if (from >= s.len) return ss0;
  return ss_mk_c(s.b.c + from, s.len - from);
}


static SS ss_to(SS s, Int to) {
  assert(to >= 0);
  if (to <= s.len) return ss0;
  return ss_mk(s.b, s.len - to);
}


static SS ss_slice(SS s, Int from, Int to) {
  assert(from >= 0);
  assert(to >= 0);
  if (from >= s.len || from >= to) return ss0;
  return ss_mk_c(s.b.c + from, to - from);
}


static Int ss_find_line_start(SS s, Int pos) {
  for_in_rev(i, pos - 1) {
    //errFL("ss_find_line_start: pos:%ld i:%ld", pos, i);
    if (s.b.c[i] == '\n') {
      return i + 1;
    }
  }
  return 0;
}


static Int ss_find_line_end(SS s, Int pos) {
  for_imn(i, pos, s.len) {
    if (s.b.c[i] == '\n') {
      return i;
    }
  }
  return s.len;
}


static SS ss_line_at_pos(SS s, Int pos) {
  Int from = ss_find_line_start(s, pos);
  Int to = ss_find_line_end(s, pos);
  assert(from >= 0);
  if (to < 0) to = s.len;
  //errFL("ss_line_at_pos: len:%ld pos:%ld from:%ld to:%ld", s.len, pos, from, to);
  return ss_slice(s, from, to);
}


static void ss_write(File f, SS s) {
  Uns items_written = fwrite(s.b.c, size_Char, (Uns)s.len, f);
  check((Int)items_written == s.len, "write SS failed: len: %ld; written: %lu", s.len, items_written);
}


static void ss_out(SS s) {
  ss_write(stdout, s);
}


static void ss_err(SS s) {
  ss_write(stderr, s);
}


// return BC must be freed.
static BM ss_src_loc_str(SS src, SS path, Int pos, Int len, Int line_num, Int col, BC msg) {
  // get source line.
  SS line = ss_line_at_pos(src, pos);
  BC nl = ss_ends_with_char(line, '\n') ? "" : "\n";
  // create underline.
  Char under[len_buffer] = {};
  if (line.len < len_buffer) { // draw underline
    for_in(i, col) under[i] = ' ';
    if (len > 0) {
      for_in(i, len) under[col + i] = '~';
    }
    else {
      under[col] = '^';
    }
  }
  // create result.
  B l = ss_copy_to_B(line); // must be freed.
  BM s;
  Int s_len = asprintf(&s,
    "%s:%ld:%ld: %s\n%s%s%s\n",
    path.b.c, line_num + 1, col + 1, msg,
    l.c, nl, under);
  free(l.m);
  check(s_len > 0, "ss_src_loc_str allocation failed: %s", msg);
  return s;
}


#pragma mark - Obj


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
static const Uns body_mask = ~tag_mask;
static const Uns flt_body_mask = max_Uns - 1;
static const Int max_Int_tagged  = (1L  << width_tagged_word) - 1;
static const Uns max_Uns_tagged  = max_Int_tagged;
static const Int shift_factor_Int = 1L << width_tag; // cannot shift signed values in C so use multiplication instead.

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

// low bit indicates 32/64 bit IEEE 754 float with low bit rounded to even.
// alternatively we could use two bits to indicate floats, and fit the remaining enum into a 3 bit tag (discarding reserved).
static const Tag ot_flt_bit = 1; // only flt words have low bit set.
static const Tag ot_unm_mask = 1 << 3; // unmanaged words have high tag bit set.
static const Tag ot_val_mask = ot_flt_bit | ot_unm_mask; // val words have at least one of these bits set.

typedef enum {
  ot_borrowed   = 0 << 1, // ref is not retained by receiver.
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
static const Uns data_word_bit = (1 << width_tag);



static void assert_obj_tags_are_valid() {
  assert(!(ot_val_mask & ot_strong));
  assert(ot_val_mask & ot_int);
}


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


typedef enum {
  st_Data_large,
  st_Vec_large,
  st_I32,
  st_I64,
  st_U32,
  st_U64,
  st_F32,
  st_F64,
  st_File,
  st_Func_host_large,
  st_Proxy,
  st_DEALLOC = tag_mask, // this value must be equal to the mask for ref_dealloc.
} Struct_tag;

static BC struct_tag_names[] = {
  "Data-large",
  "Vec-large",
  "I32",
  "I64",
  "U32",
  "U64",
  "F32",
  "F64",
  "File",
  "Func-host-large",
  "Proxy",
  "Unknown-struct-tag",
  "Unknown-struct-tag",
  "Unknown-struct-tag",
  "Unknown-struct-tag",
  "DEALLOC",
};


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

#define DEF_RC_COUNTS(RC, s, f) \
static void rc##s##_##inc##_##f(RC* rc) { \
  if (c##s##_is_pinned(rc->f)) return; \
  rc->f += count_inc; \
  if (report_pinned_counts && c##s##_is_pinned(rc->f)) { \
    errFL("object " #RC " " #s " count pinned: %p", rc); \
  } \
} \
static void rc##s##_##dec##_##f(RC* rc) { \
  if (c##s##_is_pinned(rc->f)) return; \
  assert(rc->f >= count_inc); \
  rc->f -= count_inc; \
}

#define DEF_RC_ERRS(RC, s) \
static void rc##s##_err(RC* rc) { \
  Struct_tag st = rc##s##_struct_tag(*rc); \
  errF(#RC "{st:%x:%s w:%lx sp:%d s:%lx}", \
    st, struct_tag_names[st], \
    cast(Uns, rc##s##_weak(*rc)), rc##s##_spec_tag(*rc), cast(Uns, rc##s##_strong(*rc))); \
} \
static void rc##s##_errL(RC* rc) { \
  rc##s##_err(rc); \
  err_nl(); \
}


DEF_RC_COUNTS(RCH, h, w)
DEF_RC_COUNTS(RCH, h, s)
DEF_RC_COUNTS(RCW, w, w)
DEF_RC_COUNTS(RCW, w, s)

DEF_RC_ERRS(RCH, h)
DEF_RC_ERRS(RCW, w)


// tag bits must be masked off before dereferencing all pointer types.
typedef union {
  Int i;
  Uns u;
  Flt f;
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


static void obj_errL(Obj o);


static Bool obj_is_val(Obj o) {
  return (o.u & ot_val_mask);
}


static Bool obj_is_sym(Obj o) {
  return obj_tag(o) == ot_sym_data && !(o.u & data_word_bit);
}


static Bool obj_is_data_word(Obj o) {
  return obj_tag(o) == ot_sym_data && (o.u & data_word_bit);
}


static Bool obj_is_ref(Obj o) {
  return !obj_is_val(o);
}


static void assert_obj_is_strong(Obj o) {
  assert(obj_tag(o) == ot_strong || obj_is_val(o));
}


static void assert_ref_is_valid(Obj r) {
  assert(!obj_tag(r));
  assert((*r.st & tag_mask) != st_DEALLOC);
}


static Obj ref_add_tag(Obj r, Tag t) {
  assert_ref_is_valid(r);
  assert(!(t & ot_val_mask)); // t must be a ref tag.
  return (Obj){ .u = r.u | t };
}


static Obj obj_ref_borrow(Obj o) {
  assert(obj_is_ref(o));
  return (Obj){ .u = o.u & body_mask };
}


static Obj obj_borrow(Obj o) {
  if (obj_is_ref(o)) {
    return obj_ref_borrow(o);
  }
  else return o;
}


static Struct_tag ref_struct_tag(Obj r) {
  assert_ref_is_valid(r);
  return *r.st & tag_mask;
}


static Bool ref_is_vec_large(Obj r) {
  return ref_struct_tag(r) == st_Vec_large;
}


static Bool ref_is_vec(Obj r) {
  return ref_struct_tag(r) == st_Vec_large;
}


static Bool ref_is_data_large(Obj d) {
  return ref_struct_tag(d) == st_Data_large;
}


static Bool ref_is_data(Obj d) {
  return ref_struct_tag(d) == st_Data_large;
}


static Bool ref_is_file(Obj f) {
  return ref_struct_tag(f) == st_File;
}


static Bool ref_is_func_host(Obj f) {
  return ref_struct_tag(f) == st_Func_host_large;
}


static Bool obj_is_vec(Obj o) {
  return obj_is_ref(o) && ref_is_vec(obj_ref_borrow(o));
}


static Int ref_large_len(Obj r) {
  assert_ref_is_valid(r);
  assert(ref_struct_tag(r) == st_Data_large || ref_struct_tag(r) == st_Vec_large);
  assert(r.rcwl->len > 0);
  return r.rcwl->len;
}


static Ptr ref_body(Obj r) {
  assert_ref_is_valid(r);
  return NULL; // TODO
}


static Int ref_len(Obj r) {
  return ref_large_len(r);
}


static Obj* vec_large_els(Obj v) {
  assert(ref_is_vec_large(v));
  return cast(Obj*, v.rcwl + 1); // address past rcwl.
}


static Obj* vec_els(Obj v) {
  return vec_large_els(v);
}


#define IF_OT_IS_VAL_RETURN(...) \
if (ot & ot_val_mask) return __VA_ARGS__ // value; no counting.


static Obj obj_retain_weak(Obj o) {
  Obj_tag ot = obj_tag(o);
  IF_OT_IS_VAL_RETURN(o);
  assert_ref_is_valid(o);
  errFLD("ret weak:  %p %s", o.p, struct_tag_names[ref_struct_tag(o)]);
  rcw_inc_w(o.rcw);
  return ref_add_tag(o, ot_weak);
}


static Obj obj_retain_strong(Obj o) {
  Obj_tag ot = obj_tag(o);
  IF_OT_IS_VAL_RETURN(o);
  assert_ref_is_valid(o);
  errFLD("ret strong: %p %s", o.p, struct_tag_names[ref_struct_tag(o)]);
  rcw_inc_s(o.rcw);
  return ref_add_tag(o, ot_strong);
}


static void ref_release_weak(Obj r) {
  assert_ref_is_valid(r);
  errFLD("rel weak:  %p %s", r.p, struct_tag_names[ref_struct_tag(r)]);
  rcw_dec_w(r.rcw);
}


static void ref_dealloc(Obj r);

static void ref_release_strong(Obj r) {
  assert_ref_is_valid(r);
  errFLD("rel strong: %p %s", r.p, struct_tag_names[ref_struct_tag(r)]);
  if (r.rcw->s == count_inc) { // ref count is 1.
    ref_dealloc(r);
  }
  else {
    rcw_dec_s(r.rcw);
  }
}


static void obj_release(Obj o) {
  Obj_tag ot = obj_tag(o);
  IF_OT_IS_VAL_RETURN();
  if (ot == ot_borrowed) return; // or error?
  Obj r = obj_borrow(o);
  if (ot == ot_weak) ref_release_weak(r);
  else if (ot == ot_strong) ref_release_strong(r);
  else {
    error("unknown tag: %x", ot);
 }
}


// return Obj must be tagged ot_strong or else dealloced directly.
static Obj ref_alloc(Struct_tag st, Int width) {
  assert(width > 0);
  Obj r = (Obj){.p=malloc(cast(Uns, width))};
  assert(!obj_tag(r)); // check that malloc is really aligned to the width of the tag.
  errFLD("alloc:      %p %s; %ld", r.p, struct_tag_names[st], width);
  r.rcw->w = st;
  r.rcw->s = count_inc;
#if OPT_ALLOC_COUNT
  total_allocs_ref++;
#endif
  return r;
}


static void ref_dealloc(Obj r) {
  assert_ref_is_valid(r);
  errFLD("dealloc:    %p %s", r.p, struct_tag_names[ref_struct_tag(r)]);
  check(r.rcw->w < count_inc, "attempt to deallocate object with non-zero weak count: %p", r.p);
  Struct_tag st = ref_struct_tag(r);
  if (st == st_Vec_large) {
    Obj* els = vec_large_els(r);
    for_in(i, ref_len(r)) {
      obj_release(els[i]); // TODO: make this tail recursive for deallocating long chains?
    }
  }
#if OPT_DEALLOC_MARK
  *r.st &= st_DEALLOC;
#endif
  free(r.p);
#if OPT_ALLOC_COUNT
  total_deallocs_ref++;
#endif
}


#pragma mark - Mem


typedef struct {
  Obj* els;
  Int len;
} Mem;


#define mem0 (Mem){.els=NULL, .len=0}


static Mem mem_mk(Obj* els, Int len) {
  return (Mem){.els=els, .len=len};
}


static bool mem_eq(Mem a, Mem b) {
  return a.len == b.len && memcmp(a.els, b.els, (Uns)(a.len * size_Obj)) == 0;
}


static bool mem_is_valid(Mem m) {
  return (m.len == 0) || (m.els && m.len > 0);
}


static bool mem_index_is_valid(Mem m, Int i) {
  return i >= 0 && i < m.len;
}


static void check_mem_index(Mem m, Int i) {
  assert(mem_is_valid(m));
  check(mem_index_is_valid(m, i), "invalid Mem index: %ld", i);
}


static Obj* mem_end(Mem m) {
  return m.els + m.len;
}


static Obj mem_el(Mem m, Int i) {
  check_mem_index(m, i);
  return m.els[i];
}


static Int mem_append(Mem* m, Obj o) {
  Int i = m->len++;
  m->els[i] = o;
  return i;
}


static void mem_release_els(Mem m) {
  for_in(i, m.len) {
    obj_release(m.els[i]);
  }
}


static void mem_dealloc(Mem m) {
#if OPT_ALLOC_COUNT
  if (m.els) total_deallocs_mem++;
#endif
  free(m.els);
}


static void mem_release_dealloc(Mem m) {
  mem_release_els(m);
  mem_dealloc(m);
}


// release any truncated elements, realloc memory, and zero any new elements.
// does not set m.len; use mem_resize or array_grow_cap.
static void mem_realloc(Mem* m, Int len) {
  Int old_len = m->len;
  // release any old elements.
  for_imn(i, len, old_len) {
    obj_release(m->els[i]);
  }
  // realloc.
  if (len > 0) {
#if OPT_ALLOC_COUNT
    if (!m->els) total_allocs_mem++;
#endif
    m->els = realloc(m->els, (Uns)(len * size_Obj));
    check(m->els, "realloc failed; len: %ld; width: %ld", len, size_Obj);
  }
  else if (len == 0) {
    mem_dealloc(*m); // realloc does something different, plus we must count deallocs.
    m->els = NULL;
  }
  else error("bad len: %ld", len);
  // clear any new elements.
  if (OPT_MEM_CLEAR_ELS && old_len < len) {
    memset(m->els + old_len, 0, (len - old_len) * size_Obj);
  }
}


// realloc and set size.
static void mem_resize(Mem* m, Int len) {
  mem_realloc(m, len);
  m->len = len;
}


#pragma mark - Array


typedef struct {
  Mem mem;
  Int cap;
} Array;

#define array0 (Array){.mem=mem0, .cap=0}


static bool array_is_valid(Array a) {
  return mem_is_valid(a.mem) && a.cap >= 0 && a.mem.len <= a.cap;
}


static Array array_with_len(Int len) {
  Array a = array0;
  a.cap = round_up_to_power_2(len);
  mem_realloc(&a.mem, a.cap);
  a.mem.len = len;
  return a;
}


static void array_grow_cap(Array* a) {
  a->cap = round_up_to_power_2(a->cap + 3); // minimum capacity of 4 elements.
  mem_realloc(&a->mem, a->cap);
}


static Int array_append(Array* a, Obj o) {
  if (a->mem.len == a->cap) {
    array_grow_cap(a);
  }
  return mem_append(&a->mem, o);
}


static Obj array_el(Array a, Int i) {
  return mem_el(a.mem, i);
}


#pragma mark - syms


typedef enum {
  si_NIL,
  si_VEC0,
  si_CHAIN0,
  si_END,
  si_FALSE,
  si_TRUE,
  si_COMMENT,
  si_UNQ,
  si_QUA,
  si_QUO,
  si_DO,
  si_SCOPE,
  si_LET,
  si_IF,
  si_FN,
  si_CALL,
  si_EXPA,
  si_VOID, // must be last.
} Sym_index;


static const Int shift_sym = width_tag + 1; // 1 bit reserved for Data-word flag.
static const Int sym_index_end = 1L << (size_Int * 8 - shift_sym);

#define _sym_with_index(index) (Obj){ .u = (cast(Uns, index) << shift_sym) | ot_sym_data }

static Obj sym_with_index(Int i) {
  check(i < sym_index_end, "Sym index is too large: %lx", i);
  return _sym_with_index(i);
}

static Int sym_index(Obj s) {
  assert(obj_is_sym(s));
  return s.i >> shift_sym;
}

static Bool sym_is_form(Obj s) {
  assert(obj_is_sym(s));
  return (s.u >= si_COMMENT && s.u <= si_CALL);
}


// special value constants.
#define DEF_CONSTANT(c) \
static const Obj c = _sym_with_index(si_##c)

DEF_CONSTANT(NIL);
DEF_CONSTANT(VEC0);
DEF_CONSTANT(CHAIN0);
DEF_CONSTANT(END);
DEF_CONSTANT(FALSE);
DEF_CONSTANT(TRUE);
DEF_CONSTANT(COMMENT);
DEF_CONSTANT(UNQ);
DEF_CONSTANT(QUA);
DEF_CONSTANT(QUO);
DEF_CONSTANT(DO);
DEF_CONSTANT(SCOPE);
DEF_CONSTANT(LET);
DEF_CONSTANT(IF);
DEF_CONSTANT(FN);
DEF_CONSTANT(CALL);
DEF_CONSTANT(EXPA);
DEF_CONSTANT(VOID);


// each Sym object is a small integer indexing into this array of strings.
static Array global_sym_names = array0;


#pragma mark - data


static const Obj blank = (Obj){.u = data_word_bit | ot_sym_data };


static Bool obj_is_data(Obj o) {
  return obj_is_data_word(o) || (obj_is_ref(o) && ref_is_data(obj_ref_borrow(o)));
}



#define assert_is_data(d) \
assert(d.u == blank.u || (obj_tag(d) == ot_sym_data && ref_
static Int data_len(Obj d) {
  if (d.u == blank.u) return 0; // TODO: support all data-word values.
  return ref_len(obj_borrow(d));
}


static B data_large_ptr(Obj d) {
  assert(ref_is_data_large(d));
  return (B){.m = cast(BM, d.rcwl + 1)}; // address past rcwl.
}


static B data_ptr(Obj d) {
  if (d.u == blank.u) return (B){.c=NULL}; // TODO: supoprt all data-word values.
  return data_large_ptr(d);
}


static SS data_SS(Obj d) {
  return ss_mk(data_ptr(d), data_len(d));
}


#pragma mark - Func-host





#pragma mark - new


static Obj new_int(Int i) {
  check(i >= -max_Int_tagged && i <= max_Int_tagged, "large Int values not yet suppported.");
  Int shifted = i * shift_factor_Int;
  return (Obj){ .i = (shifted | ot_int) };
}


static Obj new_uns(Uns u) {
  check(u < max_Uns_tagged, "large Uns values not yet supported.");
  return new_int(cast(Int, u));
}


static Obj data_empty(Int len) {
  Obj d = ref_alloc(st_Data_large, size_RCWL + len);
  d.rcwl->len = len;
  return d; // borrowed.
}


static Obj new_data_from_SS(SS s) {
  if (!s.len) return blank;
  Obj d = data_empty(s.len);
  memcpy(data_large_ptr(d).m, s.b.c, s.len);
  return ref_add_tag(d, ot_strong);
}


static Obj new_data_from_BC(BC bc) {
  return new_data_from_SS(ss_from_BC(bc));
}


static Obj new_data_from_path(BC path) {
  File f = fopen(path, "r");
  check(f, "could not open file: %s", path);
  fseek(f, 0, SEEK_END);
  Int len = ftell(f);
  if (!len) return blank;
  Obj d = data_empty(len);
  fseek(f, 0, SEEK_SET);
  Uns items_read = fread(data_large_ptr(d).m, size_Char, cast(Uns, len), f);
  check(cast(Int, items_read) == len, "read failed; expected len: %ld; actual bytes: %lu", len, items_read);
  fclose(f);
  return ref_add_tag(d, ot_strong);
}



static Obj new_vec_M(Mem m) {
  if (!m.len) {
    return VEC0;
  }
  Obj v = ref_alloc(st_Vec_large, size_RCWL + size_Obj * m.len);
  v.rcwl->len = m.len;
  Obj* p = vec_large_els(v);
  for_in(i, m.len) {
    Obj el = mem_el(m, i);
    assert_obj_is_strong(el);
    p[i] = el;
  }
  return ref_add_tag(v, ot_strong);
}


static Obj new_vec_OO(Obj a, Obj b) {
  Mem m = mem_mk((Obj[]){a, b}, 2);
  return new_vec_M(m);
}


static Obj new_chain_M(Mem m) {
  if (!m.len) {
    return CHAIN0;
  }
  Obj c = END;
  for_in_rev(i, m.len) {
    Obj el = mem_el(m, i);
    c = new_vec_OO(el, c);
  }
  //obj_errL(c);
  return c;
}


static Obj new_chain_OO(Obj hd, Obj tl) {
  if (tl.u == CHAIN0.u) {
    tl = CHAIN0;
  }
  return new_vec_OO(hd, tl);
}


#define ERR_SYM(s) errFL("SYM %s: u:%lu; si:%li", #s, s.u, sym_index(s));


static Obj new_sym(SS s) {
  for_in(i, global_sym_names.mem.len) {
    Obj d = obj_borrow(array_el(global_sym_names, i));
    if (ss_eq(s, data_SS(d))) {
      return sym_with_index(i);
    }
  }
  Obj d = new_data_from_SS(s);
  Int i = array_append(&global_sym_names, d);
  Obj sym = sym_with_index(i);
  //errF("NEW SYM: %ld: ", i); obj_errL(sym);
  return sym;
}


#pragma mark - File


static Obj new_File(File file) {
  Obj o = ref_alloc(st_File, size_RCWL + sizeof(File));
  File* f = cast(File*, o.rcwl + 1);
  *f = file;
  return ref_add_tag(o, ot_strong);
}


#pragma mark - Func-host


typedef struct {
  Ptr ptr;
  Int arg_count;
} ALIGNED_TO_WORD Func_host;


typedef Obj(*Func_host1)(Obj);
typedef Obj(*Func_host2)(Obj, Obj);
typedef Obj(*Func_host3)(Obj, Obj, Obj);


static Obj new_Func_host(Ptr ptr, Int arg_count) {
  Obj o = ref_alloc(st_Func_host_large, size_RCWL + sizeof(Func_host));
  Func_host* f = cast(Func_host*, o.rcwl + 1);
  f->ptr = ptr;
  f->arg_count = arg_count;
  return ref_add_tag(o, ot_strong);
}


static Obj func_host_call(Obj func, Obj args) {
  assert(ref_is_func_host(func));
  assert(ref_is_vec(args));
  Func_host* f = cast(Func_host*, func.rcwl + 1);
  Int len = ref_len(args);
  check(len == f->arg_count, "host function expects %ld arguments; received %ld",
    len, f->arg_count);
  Obj* els = vec_els(args);
  #define CALL(c, ...) case c: return (cast(Func_host##c, f->ptr))(__VA_ARGS__)
  switch (f->arg_count) {
    CALL(1, obj_borrow(els[0]));
    CALL(2, obj_borrow(els[0]), obj_borrow(els[1]));
    CALL(3, obj_borrow(els[0]), obj_borrow(els[1]), obj_borrow(els[2]));
    default: error("unsupported number of arguments to host function: %ld", f->arg_count);
  }
  #undef CALL
}


#pragma mark - type-specific


static const Obj int0 = (Obj){.i = ot_int };

static Int int_val(Obj o) {
  assert(obj_tag(o) == ot_int);
  Int i = o.i & cast(Int, body_mask);
  assert(i == 0 || i <= -shift_factor_Int || i >= shift_factor_Int);
  return i / shift_factor_Int;
}


static Flt flt_val(Obj o) {
  assert(obj_tag(o) & ot_flt_bit);
  o.u &= flt_body_mask;
  return o.f;
}


// borrow the data for a sym.
// the name is a bit confusing due to overlap with ot_sym_data (the tag for sym or data-word).
static Obj sym_data_borrow(Obj s) {
  assert(obj_is_sym(s));
  return obj_borrow(mem_el(global_sym_names.mem, sym_index(s)));
}


#define FMT_SYM(sym) cast(I32, ref_len(sym_data_borrow(sym))), data_large_ptr(sym_data_borrow(sym))


static Obj vec_hd(Obj v) {
  assert(ref_is_vec(v));
  Obj* els = vec_els(v);
  Obj el = els[0];
  return obj_borrow(el);
}


static Obj vec_tl(Obj v) {
  assert(ref_is_vec(v));
  Obj* els = vec_els(v);
  Int len = ref_len(v);
  Obj el = els[len - 1];
  return obj_borrow(el);
}


#define vec_a vec_hd


static Obj vec_b(Obj v) {
  assert(ref_is_vec(v));
  Obj* els = vec_els(v);
  Int len = ref_len(v);
  assert(len > 1);
  Obj el = els[1];
  return obj_borrow(el);
}


typedef enum {
  vs_vec,
  vs_chain,
  vs_chain_blocks,
} Vec_shape;


static Vec_shape vec_shape(Obj v) {
  assert(ref_is_vec(v));
  Vec_shape s = vs_chain;
  loop {
    Obj last = vec_tl(v);
    if (last.u == END.u) return s;
    if (!obj_is_ref(last)) return vs_vec;
    Obj r = obj_ref_borrow(last);
    if (!ref_is_vec(r)) return vs_vec;
    if (ref_len(v) != 2) s = vs_chain_blocks;
    v = r;
  }
}


static Bool is_true(Obj o) {
  if (obj_is_sym(o)) {
    return (sym_index(o) >= si_TRUE);
  }
#define F(c) if (o.u == c.u) return false;
  F(blank);
  F(int0);
#undef F
  return true;
}


#pragma mark - write repr


static void write_repr_data(File f, Obj d) {
  assert(ref_is_data(d));
  Int l = d.rcwl->len;
  BC p = data_large_ptr(d).c;
  fputc('\'', f);
  for_in(i, l) {
    Char c = p[i];
    switch (c) {
      case '\a': fputc('\\', f); fputc('a', f);  continue; // bell - BEL
      case '\b': fputc('\\', f); fputc('b', f);  continue; // backspace - BS
      case '\f': fputc('\\', f); fputc('f', f);  continue; // form feed - FF
      case '\n': fputc('\\', f); fputc('n', f);  continue; // line feed - LF
      case '\r': fputc('\\', f); fputc('r', f);  continue; // carriage return - CR
      case '\t': fputc('\\', f); fputc('t', f);  continue; // horizontal tab - TAB
      case '\v': fputc('\\', f); fputc('v', f);  continue; // vertical tab - VT
      case '\\': fputc('\\', f); fputc('\\', f); continue;
      case '\'': fputc('\\', f); fputc('\'', f); continue;
      case '"':  fputc('\\', f); fputc('"', f);  continue;
    }
    // TODO: escape non-printable characters
    fputc(c, f);
  }
  fputc('\'', f);
}


static void write_repr_obj(File f, Obj o);


static void write_repr_vec_vec(File f, Obj v) {
  assert(ref_is_vec(v));
  Int len = ref_len(v);
  Obj* els = vec_els(v);
  Obj hd = els[0];
  if (hd.u == CALL.u && ref_len(v) == 2 && obj_is_vec(vec_tl(v))) { // call
    fputs("(", f);
    for_imn(i, 1, len) { // skip the CALL sym.
      if (i > 1) fputs(" ", f);
      write_repr_obj(f, els[i]);
    }
    fputs(")", f);
  }
  else {
    fputs("{", f);
    for_in(i, len) {
      if (i) fputs(" ", f);
      write_repr_obj(f, els[i]);
    }
    fputs("}", f);
  }
}


static void write_repr_chain(File f, Obj c) {
  assert(ref_is_vec(c));
  fputs("[", f);
  Bool first = true;
  loop {
    if (first) first = false;
    else fputs(" ", f);
    write_repr_obj(f, vec_hd(c));
    Obj tl = vec_tl(c);
    if (tl.u == END.u) break;
    assert(obj_is_vec(tl));
    c = obj_ref_borrow(tl);
  }
  fputs("]", f);
}


static void write_repr_chain_blocks(File f, Obj c) {
  assert(ref_is_vec(c));
  fputs("[", f);
  loop {
    Obj* els = vec_els(c);
    Int len = ref_len(c);
    fputs("|", f);
    for_in(i, len - 1) {
      if (i) fputs(" ", f);
      write_repr_obj(f, els[i]);
    }
    Obj tl = els[len - 1];
    if (tl.u == END.u) break;
    assert(obj_is_vec(tl));
    c = obj_ref_borrow(tl);
  }
  fputs("]", f);
}


static void write_repr_vec(File f, Obj v) {
  assert(ref_is_vec(v));
  switch (vec_shape(v)) {
    case vs_vec:          write_repr_vec_vec(f, v);       return;
    case vs_chain:        write_repr_chain(f, v);         return;
    case vs_chain_blocks: write_repr_chain_blocks(f, v);  return;
  }
  assert(0);
}


static void write_repr_obj(File f, Obj o) {
  Obj_tag ot = obj_tag(o);
  if (ot & ot_flt_bit) {
    fprintf(f, "(Flt %f)", flt_val(o));
  }
  else if (ot == ot_int) {
    fprintf(f, "%ld", int_val(o));
  }
  else if (ot == ot_sym_data) {
    if (o.u & data_word_bit) { // data-word
      // TODO: support all word values.
      assert(o.u == blank.u);
      fputs("''", f);
    }
    else { // sym
      Obj s = sym_data_borrow(o);
      write_repr_data(f, s);
    }
  }
  else if (ot == ot_reserved0) {
    fprintf(f, "(reserved0 %p)", o.p);
  }
  else if (ot == ot_reserved1) {
    fprintf(f, "(reserved1 %p)", o.p);
  }
  else {
    Obj r = obj_ref_borrow(o);
    switch (ref_struct_tag(r)) {
      case st_Data_large: write_repr_data(f, r);  break;
      case st_Vec_large:  write_repr_vec(f, r);   break;
      case st_I32:        fputs("(I32 ?)", f);    break;
      case st_I64:        fputs("(I64 ?)", f);    break;
      case st_U32:        fputs("(U32 ?)", f);    break;
      case st_U64:        fputs("(U64 ?)", f);    break;
      case st_F32:        fputs("(F32 ?)", f);    break;
      case st_F64:        fputs("(F64 ?)", f);    break;
      case st_File:       fputs("(File ?)", f);   break;
      case st_Func_host_large: fputs("(Func-host ?)", f); break;
      case st_Proxy:      fputs("(Proxy ?)", f);  break;
      case st_DEALLOC:    error("deallocated object is still referenced: %p", r.p);
    }
  }
  err_flush(); // TODO: for debugging only?
}


static void obj_errL(Obj o) {
  write_repr_obj(stderr, o);
  err_nl();
}


#define error_obj(msg, o) { \
errF("%s error: " msg ": ", (process_name ? process_name : __FILE__)); \
obj_errL(o); \
exit(1); \
}

#define check_obj(condition, msg, o) { if (!(condition)) error_obj(msg, o); }


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


#pragma mark - host functions


static Obj func_host_write(Obj file, Obj data) {
  check_obj(ref_is_file(file), "write expected File object; found", file);
  check_obj(ref_is_data(data), "write expected Data object; found", data);

  return new_int(0);
}


#pragma mark - Parser


// main parser object holds the input string, parse state, and source location info.
typedef struct {
  SS  src;  // input string.
  SS  path; // input path for error reporting.
  Int pos;
  Int line;
  Int col;
  BC e; // error message.
} Parser;


// character terminates a syntactic sequence.
static Bool char_is_seq_term(Char c) {
  return
  c == ')' ||
  c == ']' ||
  c == '}' ||
  c == '>';
}

// character terminates an atom.
static Bool char_is_atom_term(Char c) {
  return c == '|' || isspace(c) || char_is_seq_term(c);
}


static void parser_err(Parser* p) {
  errF("%s:%ld:%ld (%ld): ", p->path.b.c, p->line + 1, p->col + 1, p->pos);
  if (p->e) errF("\nerror: %s\nobj:  ", p->e);
}


static void parser_errL(Parser* p) {
  parser_err(p);
  err_nl();
}


 __attribute__((format (printf, 2, 3)))
static Obj parse_error(Parser* p, BC fmt, ...) {
  va_list args;
  va_start(args, fmt);
  BM msg;
  Int msg_len = vasprintf(&msg, fmt, args);
  va_end(args);
  check(msg_len >= 0, "parse_error allocation failed: %s", fmt);
  // e must be freed by parser owner.
  p->e = ss_src_loc_str(p->src, p->path, p->pos, 0, p->line, p->col, msg);
  raw_dealloc(msg);
  return VOID;
}


#define parse_check(condition, fmt, ...) \
if (!(condition)) return parse_error(p, fmt, ##__VA_ARGS__)


#define PP  (p->pos < p->src.len)
#define PP1 (p->pos < p->src.len - 1)
#define PP2 (p->pos < p->src.len - 2)

#define PC  p->src.b.c[p->pos]
#define PC1 p->src.b.c[p->pos + 1]
#define PC2 p->src.b.c[p->pos + 2]

#define P_ADV(n) { p->pos += n; p->col += n; }
#define P_ADV1 P_ADV(1)


static U64 parse_U64(Parser* p) {
  Char c = PC;
  I32 base = 0;
  if (c == '0' && PP1) {
    Char c1 = PC1;
    switch (c1) {
      case 'b': base = 2;   break;
      case 'q': base = 4;   break;
      case 'o': base = 8;   break;
      case 'd': base = 10;  break;
      case 'h': base = 16;  break;
    }
    if (base) {
      P_ADV(2);
    }
    else {
      base = 10;
    }
  }
  BC start = p->src.b.c + p->pos;
  BM end;
  U64 u = strtoull(start, &end, base);
  int en = errno;
  if (en) {
    parse_error(p, "malformed number literal: %s", strerror(en));
    return 0;
  }
  assert(end > start); // strtoull man page is a bit confusing as to the precise semantics.
  Int n = end - start;
  P_ADV(n);
  if (PP && !char_is_atom_term(PC)) {
    parse_error(p, "invalid number literal terminator: %c", PC);
    return 0;
  }
  return u;
}


static Obj parse_uns(Parser* p) {
  return new_uns(parse_U64(p));
}


static Obj parse_int(Parser* p, Int sign) {
  assert(PC == '-' || PC == '+');
  P_ADV1;
  U64 u = parse_U64(p);
  parse_check(u <= max_Int, "signed number literal is too large");
  return new_int((I64)u * sign);
}


static Obj parse_sym(Parser* p) {
  assert(PC == '_' || isalpha(PC));
  Int pos = p->pos;
  loop {
    P_ADV1;
    if (!PP) break;
    Char c = PC;
    if (!(c == '-' || c == '_' || isalnum(c))) break;
  }
  SS s = ss_slice(p->src, pos, p->pos);
  return new_sym(s);
}


static Obj parse_comment(Parser* p) {
  assert(PC == '#');
  Int pos_report = p->pos;
  do {
    P_ADV1;
    if (!PP) {
      p->pos = pos_report; // better error message;
      return parse_error(p, "unterminated comment (add newline)");
    }
  }  while (PC == ' ');
  Int pos_start = p->pos;
  do {
    P_ADV1;
    if (!PP) {
      p->pos = pos_report; // better error message;
      return parse_error(p, "unterminated comment (add newline)");
    }
  }  while (PC != '\n');
  SS s = ss_slice(p->src, pos_start, p->pos);
  Obj d = new_data_from_SS(s);
  return new_vec_OO(COMMENT, d);
}


static Obj parse_data(Parser* p, Char q) {
  assert(PC == q);
  Int pos_open = p->pos; // for error reporting.
  SS s = ss_alloc(16); // could declare as static to reduce reallocs.
  Int i = 0;

#define APPEND(c) { \
  if (i == s.len) ss_realloc(&s, round_up_to_power_2(s.len + 3)); \
  assert(i < s.len); \
  s.b.m[i++] = c; \
}

  Bool escape = false;
  loop {
    P_ADV1;
    if (!PP) {
      p->pos = pos_open; // better error message.
      return parse_error(p, "unterminated string literal");
    }
    Char c = PC;
    if (escape) {
      escape = false;
      Char ce = c;
      switch (c) {
        case '0': ce = 0;     break; // NULL
        case 'a': ce = '\a';  break; // bell - BEL
        case 'b': ce = '\b';  break; // backspace - BS
        case 'f': ce = '\f';  break; // form feed - FF
        case 'n': ce = '\n';  break; // line feed - LF
        case 'r': ce = '\r';  break; // carriage return - CR
        case 't': ce = '\t';  break; // horizontal tab - TAB
        case 'v': ce = '\v';  break; // vertical tab - VT
        case '\\':  break;
        case '\'':  break;
        case '"':   break;
        case 'x': // ordinal escapes not yet implemented. \xx byte value.
        case 'u': // \uuuu unicode.
        case 'U': // \UUUUUU unicode. 6 or 8 chars? utf-8 is restricted to end at U+10FFFF.
        default: APPEND('\\'); // not a valid escape code.
      }
      APPEND(ce);
    }
    else {
      if (c == q) break;
      if (c == '\\') escape = true;
      else APPEND(c);
    }
  }
  #undef APPEND
  P_ADV1; // past closing quote.
  Obj d = new_data_from_SS(ss_mk(s.b, i));
  ss_dealloc(s);
  return d;
}


static Obj parse_expr(Parser* p);


static Bool parse_space(Parser* p) {
  while (PP) {
    Char c = PC;
    //parser_err(p); errFL("space? '%c'", c);
    switch (c) {
      case ' ':
        P_ADV1;
        continue;
      case '\n':
        p->pos++;
        p->line++;
        p->col = 0;
        continue;
      case '\t':
        parse_error(p, "illegal tab character");
        return false;
      default:
        if (isspace(c)) {
          parse_error(p, "illegal space character: '%c' (%02x)", c, c);
          return false;
        }
        return true;
    }
  }
  return false; // EOS.
}


static Bool parse_terminator(Parser* p, Char t) {
  if (PC != t) {
    parse_error(p, "expected terminator: '%c'", t);
    return false;
  }
  P_ADV1;
  return true;
}

static Bool parser_has_next_expr(Parser* p) {
  return parse_space(p) && !char_is_seq_term(PC);
}


// return Mem must be freed.
static Mem parse_seq(Parser* p) {
  Array a = array0;
  while (parser_has_next_expr(p)) {
    Obj o = parse_expr(p);
    if (p->e) {
      mem_release_dealloc(a.mem);
      return mem0;
    }
    array_append(&a, o);
  }
  return a.mem;
}


// return Mem must be freed.
static Mem parse_blocks(Parser* p) {
  Array a = array0;
  while (parser_has_next_expr(p)) {
    Char c = PC;
    Obj o;
    if (c == '|') {
      P_ADV1;
      Mem m = parse_seq(p);
      if (p->e) {
        mem_release_dealloc(a.mem);
        return mem0;
      }
      o = new_vec_M(m);
    }
    else {
      o = parse_expr(p);
      if (p->e) {
        mem_release_dealloc(a.mem);
        return mem0;
      }
    }
    array_append(&a, o);
  }
  return a.mem;
}


#define P_ADV_TERM(t) \
if (p->e || !parse_terminator(p, t)) { \
  mem_release_dealloc(m); \
  return VOID; \
}


static Obj parse_call(Parser* p) {
  P_ADV1;
  Mem m = parse_seq(p);
  P_ADV_TERM(')');
  Obj v = new_vec_M(m);
  mem_dealloc(m);
  Obj c = new_vec_OO(CALL, v);
  return c;
}


static Obj parse_expa(Parser* p) {
  P_ADV1;
  Mem m = parse_seq(p);
  P_ADV_TERM('>');
  Obj v = new_vec_M(m);
  mem_dealloc(m);
  Obj e = new_vec_OO(EXPA, v);
  return e;
}


static Obj parse_vec(Parser* p) {
  P_ADV1;
  Mem m = parse_seq(p);
  P_ADV_TERM('}');
  Obj v = new_vec_M(m);
  mem_dealloc(m);
  return v;
}


static Obj parse_chain(Parser* p) {
  P_ADV1;
  Mem m = parse_blocks(p);
  P_ADV_TERM(']');
  Obj c = new_chain_M(m);
  mem_dealloc(m);
  return c;
}


static Obj parse_qua(Parser* p) {
  assert(PC == '`');
  P_ADV1;
  Obj o = parse_expr(p);
  return new_vec_OO(QUA, o);
}


// parse an expression.
static Obj parse_expr_sub(Parser* p) {
  Char c = PC;
  switch (c) {
    case '(':   return parse_call(p);
    case '<':   return parse_expa(p);
    case '{':   return parse_vec(p);
    case '[':   return parse_chain(p);
    case '`':   return parse_qua(p);
    case '\'':  return parse_data(p, '\'');
    case '"':   return parse_data(p, '"');
    case '#':   return parse_comment(p);
    case '+':
      if (isdigit(PC1)) return parse_int(p, 1);
      break;
    case '-':
      if (isdigit(PC1)) return parse_int(p, -1);
      break;

  }
  if (isdigit(c)) {
    return parse_uns(p);
  }
  if (c == '_' || isalpha(c)) {
    return parse_sym(p);
  }
  return parse_error(p, "unexpected character");
}


static Obj parse_expr(Parser* p) {
  Obj o = parse_expr_sub(p);
#if VERBOSE_PARSE
  parser_err(p); obj_errL(o);
#endif
  return o;
}


// caller must free e.
static Obj parse_src(Obj path, Obj src, BM* e) {
  Parser p = (Parser) {
    .path=data_SS(path),
    .src=data_SS(src),
    .pos=0,
    .line=0,
    .col=0,
    .e=NULL,
  };
  Mem m = parse_seq(&p);
  Obj o;
  if (p.e) {
    o = VOID;
  }
  else if (p.pos != p.src.len) {
    parse_error(&p, "parsing terminated early");
    o = VOID;
  }
  else {
    Obj c = new_vec_M(m);
    o = new_vec_OO(DO, c);
  }
  mem_dealloc(m);
  *e = cast(BM, p.e);
  return o;
}


#pragma mark - env


static Obj env_get(Obj env, Obj sym) {
  assert_ref_is_valid(env);
  while (env.u != END.u) {
    Obj frame = vec_hd(env);
    if (frame.u != CHAIN0.u) {
      while (frame.u != END.u) {
        Obj binding = vec_hd(frame);
        assert(ref_len(binding) == 3);
        Obj key = vec_a(binding);
        if (key.u == sym.u) {
          return vec_b(binding);
        }
        frame = vec_tl(frame);
      }
    }
    env = vec_tl(env);
  }
  return VOID; // lookup failed.
}


#pragma mark - cont


typedef struct _Step Step;
typedef Step (^Cont)(Obj);

struct _Step {
  Cont cont;
  Obj val;
};


#define Step0 (Step){ .cont=NULL, .val=VOID }

static Step step_mk(Cont cont, Obj val) {
  return (Step){.cont=cont, .val=val};
}

#define STEP(...) return step_mk(__VA_ARGS__)


#pragma mark - eval


static Step eval(Cont cont, Obj env, Obj code);


static Step eval_sym(Cont cont, Obj env, Obj code) {
  assert(obj_is_sym(code));
  if (code.u < VOID.u) STEP(cont, code); // constants are self-evaluating.
  if (code.u == VOID.u) error("cannot eval VOID");
  Obj val = env_get(env, code);
  if (val.u == VOID.u) { // lookup failed.
    error_obj("lookup error", code);
  }
  STEP(cont, val);
}


static Step eval_QUO(Cont cont, Obj env, Int len, Obj* args) {
  check(len == 1, "QUO requires 1 argument; found %ld", len);
  STEP(cont, args[0]);
}


static Step eval_DO(Cont cont, Obj env, Int len, Obj* args) {
  check(len == 1, "DO requires 1 argument; found %ld", len);
  Obj body = args[0];
  if (body.u == VEC0.u) {
    STEP(cont, VOID);
  }
  check_obj(obj_is_vec(body), "DO argument must be a vec; found", body);
  body = obj_ref_borrow(body);
  Int body_len = ref_len(body);
  Obj* body_els = vec_els(body);
  Cont next = cont;
  for_in_rev(i, body_len) {
    Obj a = obj_borrow(body_els[i]);
    Cont c = ^(Obj o){
      //errF("cont DO %li: ", i); obj_errL(a);
      return eval(next, env, a);
    };
    next = Block_copy(c);
  }
  STEP(next, VOID);
}


static Step eval_SCOPE(Cont cont, Obj env, Int len, Obj* args) {
  check(len == 1, "SCOPE requires 1 argument; found %ld", len);
  Obj body = args[0];
  // TODO: create new env frame.
  return eval(cont, env, body);
}


static Step eval_LET(Cont cont, Obj env, Int len, Obj* args) {
  STEP(cont, VOID);
}


static Step eval_IF(Cont cont, Obj env, Int len, Obj* args) {
  check(len == 3, "IF requires 3 arguments; found %ld", len);
  Obj pred  = obj_borrow(args[0]);
  Obj then_ = obj_borrow(args[1]);
  Obj else_ = obj_borrow(args[2]);
  Cont c = ^(Obj o){
    if (is_true(o)) {
      return eval(cont, env, then_);
    }
    else {
      return eval(cont, env, else_);
    }
  };
  return eval(Block_copy(c), env, pred);
}


static Step eval_FN(Cont cont, Obj env, Int len, Obj* args) {
  STEP(cont, VOID);
}


static Step eval_CALL(Cont cont, Obj env, Int len, Obj* args) {
  STEP(cont, VOID);
}


static Step eval_Vec_large(Cont cont, Obj env, Obj code) {
  Int len = ref_large_len(code);
  Obj* els = vec_large_els(code);
  Obj form = obj_borrow(els[0]);
  Obj* args = els + 1;
  Tag ot = obj_tag(form);
  if (ot == ot_sym_data && !(ot & data_word_bit)) {
    Int si = sym_index(form);
#define EVAL_FORM(s) case si_##s: return eval_##s(cont, env, len - 1, args)
    switch (si) {
      EVAL_FORM(QUO);
      EVAL_FORM(DO);
      EVAL_FORM(SCOPE);
      EVAL_FORM(LET);
      EVAL_FORM(IF);
      EVAL_FORM(FN);
      EVAL_FORM(CALL);
    }
#undef EVAL_FORM
  }
  error_obj("cannot call object", code);
}


static const BC trace_eval_prefix   = "▿ "; // during trace, printed before each eval; white_down_pointing_small_triangle.
static const BC trace_cont_prefix   = "◦ "; // during trace, printed before calling continuation; white_bullet.
static const BC trace_apply_prefix  = "▹ "; // called before each call apply;  white_right_pointing_small_triangle.


static Step eval(Cont cont, Obj env, Obj code) {
  Obj_tag ot = obj_tag(code);
  if (ot & ot_flt_bit || ot == ot_int) {
    STEP(cont, code); // self-evaluating.
  }
  if (ot == ot_sym_data) {
    if (ot & data_word_bit) {
      STEP(cont, code); // self-evaluating.
    }
    return eval_sym(cont, env, code);
  }
  if (ot == ot_reserved0 || ot == ot_reserved1) {
    error_obj("cannot eval reserved object", code);
  }
  assert_ref_is_valid(code);
  switch (ref_struct_tag(code)) {
    case st_Vec_large:
      return eval_Vec_large(cont, env, code);
    case st_Data_large:
    case st_I32:
    case st_I64:
    case st_U32:
    case st_U64:
    case st_F32:
    case st_F64:
      STEP(cont, code);
    case st_File:             error_obj("cannot eval File object", code);
    case st_Func_host_large:  error_obj("canont eval Func-host object", code);
    case st_Proxy:            error_obj("cannot eval proxy object", code);
    case st_DEALLOC:          error_obj("cannot eval deallocated object", code);
  }
}


static Obj eval_loop(Obj env, Obj code) {
  Step s = eval(NULL, env, code);
  while (s.cont) {
#if VERBOSE_EVAL
    err(trace_cont_prefix); obj_errL(s.val);
#endif
    s = s.cont(s.val);
    Block_release(s.cont);
  }
  return s.val;
}


static void parse_and_eval(Obj env, Obj path, Obj src, Array* sources, Bool out_val) {
    BM e = NULL;
    Obj code = parse_src(obj_borrow(path), obj_borrow(src), &e);
    if (e) {
      err("parse error: ");
      errL(e);
      raw_dealloc(e);
      exit(1);
    }
    else {
      //obj_errL(code);
      array_append(sources, new_vec_OO(path, src));
      Obj val = eval_loop(env, obj_borrow(code));
      if (out_val) {
        write_repr_obj(stdout, val);
        out_nl();
      }
      obj_release(val);
    }
    obj_release(code);
}


#pragma mark - main


static void init_syms() {
  assert(global_sym_names.mem.len == 0);
  Obj sym;
#define SBC(bc, si) sym = new_sym(ss_from_BC(bc)); assert(sym_index(sym) == si)
#define S(s) SBC(#s, si_##s)
  S(NIL);
  S(VEC0);
  S(CHAIN0);
  S(END);
  SBC("false", si_FALSE);
  SBC("true", si_TRUE);
  S(COMMENT);
  S(UNQ);
  S(QUA);
  S(QUO);
  S(DO);
  S(SCOPE);
  S(LET);
  S(IF);
  S(FN);
  S(CALL);
  S(EXPA);
  S(VOID);
#undef SBC
#undef S
}


int main(int argc, BC argv[]) {
  
  assert_host_basic();
  assert_obj_tags_are_valid();
  assert(size_Obj == size_Word);
  assert(int_val(new_int(-1)) == -1);
  set_process_name(argv[0]);
  init_syms();
  
  //vol_err = 1;
  // parse arguments.
  BC paths[len_buffer];
  BC expr = NULL;
  Bool out_expr = false;
  Int path_count = 0;
  for_imn(i, 1, argc) {
    BC arg = argv[i];
    errFLD("   %s", arg);
    if (bc_eq(arg, "-v")) {
      vol_err = 1;
    }
    else if (bc_eq(arg, "-e") || bc_eq(arg, "-E")) {
      check(!expr, "multiple expression arguments");
      i++;
      check(i < argc, "missing expression argument");
      expr = argv[i];
      out_expr = bc_eq(arg, "-e");
    }
    else {
      check(path_count < len_buffer, "exceeded max paths: %d", len_buffer);
      paths[path_count++] = arg;
    }
  }

  Obj global_env = new_vec_OO(CHAIN0, END);
  Array sources = array0;
  
  // handle arguments.
  for_in(i, path_count) {
    Obj path = new_data_from_BC(paths[i]);
    Obj src = new_data_from_path(paths[i]);
    parse_and_eval(obj_ref_borrow(global_env), path, src, &sources, false);
  }
  if (expr) {
    Obj path = new_data_from_BC("<expr>");
    Obj src = new_data_from_BC(expr);
    parse_and_eval(global_env, path, src, &sources, out_expr);
  }
  obj_release(global_env);
  
#if OPT_ALLOC_COUNT
  mem_release_dealloc(global_sym_names.mem);
  mem_release_dealloc(sources.mem);

#define CHECK_ALLOCS(group) \
  if (vol_err || total_allocs_##group != total_deallocs_##group) { \
    errFL("==== PLOY ALLOC STATS: " #group ": alloc: %ld; dealloc: %ld; diff = %ld", \
     total_allocs_##group, total_deallocs_##group, total_allocs_##group - total_deallocs_##group); \
  }

  CHECK_ALLOCS(raw)
  CHECK_ALLOCS(mem)
  CHECK_ALLOCS(ref)

#endif // OPT_ALLOC_COUNT

  return 0;
}

