// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "01-Utf8.c"


/*

tagged objects are used to allow for polymorphic behavior in c functions:
- dealloc: must be able to clean up memory in a generic fashion.
- repr: print code representation of data structures.
- eval.

4 tag bits are guaranteed by 16-byte aligned malloc.
value types:
0000: managed pointer available for use by hosted system.
0001: ?
0010: Int. (28/60 bit signed int); 28 bits is +/-134M.
0011; Special values (see below).

reference types:
each ref must be marked as borrowed, strong, or weak.
rr00: Word: single unmanaged 8-byte word; any of I32, U32, F32, I64, U64, F64.
rr01: Str.
rr10: Vec:
rr11: ? could be 32-byte aligned to gain extra tag bit.

Special Values: 4 bit (1 nibble) tag, 28/60 bit (7/15 nibble) value.
the next bit is the short-string bit.
if set, the next 3 bits are the length, and the remaining 3/7 bytes are the string bytes.
otherwise, the even values count up through a number of constants, to the first Sym value.

the remaining space dedicated to Sym index values.
  0000 000t: False.
  0000 002t: True.
  ...
  0000 0??t: Symbol start marker.
  FFFF FFEt: Symbol end.

  0000 001t: '' (the empty string).
  0000 011t: 1 char string.
  ...

tinyscheme has the following additional tag types:
proc: ?
closure: ?
continuation: no ploy continuation objects.
foreign: ?
character: represented as Int?
port: ?
vector: ?
macro: ?
promise: ?
environment: represented as list of lists?

*/


// tagged pointer value mask.
const Uns tag_mask = 7UL;

typedef U8 Tag;

typedef struct {
  Tag t:4;  // tag.
  Int i:60; // shifted int value or ignored.
} Val;

typedef enum {
  val_tag_managed   = 0,
  val_tag_reserved  = 1,
  val_tag_int       = 2,
  val_tag_special   = 3,
} Val_tag;

typedef enum {
  struct_tag_word = 0,
  struct_tag_str  = 1,
  struct_tag_vec  = 2,
  struct_tag_xxx  = 3,
} Struct_tag;
const U8 struct_tag_mask = 3;

typedef enum {
  ref_tag_value    = 0,
  ref_tag_borrowed = 1 << 2,
  ref_tag_strong   = 2 << 2,
  ref_tag_weak     = 3 << 2,
} Ref_tag;
const U8 ref_tag_mask = 3 << 2;


// special value constants.
#define DEF_CONSTANT(i, c) const Uns c = (i << 4) | val_tag_special

DEF_CONSTANT(0, c_f);
DEF_CONSTANT(1, c_t);
DEF_CONSTANT(2, c_none);
DEF_CONSTANT(3, c_empty);
DEF_CONSTANT(4, c_void);
DEF_CONSTANT(5, c_sym);


// reference count struct.
// tag is placed last so that wc is accessed directly on little-endian systems?
typedef struct {
  U32 s;    // strong count.
  U32 w:29; // weak count.
  U8  t:3;  // type-specific tag; number of bits constrains possible variants of Word.
} ALIGNED_8 RC;
const Uns width_RC = sizeof(RC);
const Uns max_rcs = max_Uns;
const Uns max_rcw = (1<<29) - 1;

const Uns max_str_len_small = width_Uns - 1;
const Uns max_str_len_med   = max_str_len_small + (1 << 3);

// TODO: short string struct.

// struct header for Array-like objects with variable-length allocations.
// len indicates additional allocated bytes (for Str) or words (for Vec).
// len is declared as Int to preserve the alignment of subsequent words.
typedef struct {
  RC rc;
  Int len;
} ALIGNED_8 SH;
const Uns width_SH = sizeof(SH);

// tag bits must be masked off before dereferencing all pointer types.
typedef union {
  Val   v; // split tag, int.
  Uns   u;
  void* p;
  RC*   rc;
  SH*   sh;
} Obj;


Val_tag val_tag(Obj o) {
  return o.v.t;
}

Struct_tag struct_tag(Obj o) {
  return val_tag(o) & struct_tag_mask;
}

Ref_tag ref_tag(Obj o) {
  return val_tag(o) & ref_tag_mask;
}


#define assert_ref_masked(s) assert(s.v.t == 0)


Obj strip_tag(Obj o) {
  return (Obj){ .u = o.u & ~tag_mask };
}

Obj add_tags(Obj s, Struct_tag st, Ref_tag rt) {
  return (Obj){ .u = s.u | st | rt };
}


#define DEF_s_st_rt(o) \
  Obj s = strip_tag(o); \
  Struct_tag st = struct_tag(o); \
  Ref_tag rt = ref_tag(o); \


#define check_str_or_vec(st) check(st == struct_tag_str || st == struct_tag_vec, "bad struct")


Int len_Str(Obj s, Struct_tag st) {
  assert_ref_masked(s);
  assert(st == struct_tag_str);
  return s.rc->t ? s.rc->t + max_str_len_small : s.sh->len;
}

Int len_Vec(Obj s, Struct_tag st) {
  assert_ref_masked(s);
  assert(st == struct_tag_vec);
  return s.rc->t ? s.rc->t + 1 : s.sh->len;
}


Utf8 utf8_ptr(Obj s, Struct_tag st) {
  assert_ref_masked(s);
  check(st == struct_tag_str, "bad struct");
  return s.rc->t ? (Utf8)(s.rc + 1) : (Utf8)(s.sh + 1);
}


Obj* el_ptr(Obj s, Struct_tag st) {
  assert_ref_masked(s);
  check(st == struct_tag_vec, "bad struct");
  return s.rc->t ? (Obj*)(s.rc + 1) : (Obj*)(s.sh + 1);
}


Obj retain_strong(Obj o) {
  DEF_s_st_rt(o);
  if (rt == ref_tag_value) return o;
  if (rt == ref_tag_strong) error("object ref is already strong: %p", o.p);
  if (rt == ref_tag_weak)   error("object ref is already weak: %p", o.p);
  if (s.rc->s < max_rcs) {
    if (s.rc->s == max_rcs - 1) {
      errL("object strong ref-count maxed out: %p", s.p);
    }
    s.rc->s++;
  }
  return add_tags(s, st, ref_tag_strong);
}


Obj retain_weak(Obj o) {
  DEF_s_st_rt(o);
  if (rt == ref_tag_value) return o;
  if (rt == ref_tag_strong) error("object ref is already strong: %p", o.p);
  if (rt == ref_tag_weak)   error("object ref is already weak: %p", o.p);
  if (s.rc->s < max_rcs) {
    if (s.rc->s == max_rcs - 1) {
      errL("object weak ref-count maxed out: %p", s.p);
    }
    error("not yet implemented");
  }
  return add_tags(s, st, ref_tag_weak);
}


void dealloc(Obj s, Struct_tag st);


void release_strong(Obj s, Struct_tag st) {
  assert_ref_masked(s);
  assert(s.rc->s > 0); // over-release.
  if (s.rc->s == 1) {
    dealloc(s, st);
  }
  else if (s.rc->s < max_rcs) { // maxed refcounts stay pinned forever.
    s.rc->s--;
  }
}


NORETURN release_weak(Obj s, Struct_tag st) {
  error("not yet implemented");
}


void release(Obj o) {
  DEF_s_st_rt(o);
  switch (rt) {
    case ref_tag_value:     return;
    case ref_tag_borrowed:  return; // or error?
    case ref_tag_weak:      release_weak(s, st);
    case ref_tag_strong:    release_strong(s, st);
  }
}


void dealloc(Obj s, Struct_tag st) {
  if (st == struct_tag_vec) {
    Obj* els = el_ptr(s, st);
    for_in(i, len_Vec(s, st)) {
      release(els[i]);
    }
  }
  free(s.p);
}

