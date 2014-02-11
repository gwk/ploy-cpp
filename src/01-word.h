// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// basic types, macros, and functions.


#if !DEBUG
#define NDEBUG 1 // omit assertions.
#endif

// exclude the standard libraries when preprocessing the source for inspection;
// see sh/preprocess.sh.
#ifndef SKIP_LIB_INCLUDES
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#endif

// include the generated source here, before 'char' becomes an illegal type.
#include "ploy-core.h"


// on ref object dealloc, set the reference count to zero to aid in debugging overelease bugs.
#ifndef OPT_DEALLOC_MARK
#define OPT_DEALLOC_MARK DEBUG
#endif

// on ref object dealloct, do not actually call free to aid in debugging overelease bugs.
#ifndef OPT_DEALLOC_PRESERVE
#define OPT_DEALLOC_PRESERVE DEBUG
#endif

// count all heap allocations and deallocations.
#ifndef OPT_ALLOC_COUNT
#define OPT_ALLOC_COUNT DEBUG
#endif

// write invalid or unlikely values into unused portions of heap allocations.
// this helps detect illegal reads from uninitialized and dead regions,
// and also helps detect errors in allocation code,
// because we immediately attempt to write to the entire allocation.
// for this reason it is not totally redundant with the MallocScribble and MallocPreScribble
// environment variables on OSX, but it is completely compatible.
#ifndef OPT_ALLOC_SCRIBBLE
#define OPT_ALLOC_SCRIBBLE DEBUG
#endif

// verbose logging to aid in various debugging scenarios.
#define VERBOSE 0
#define VERBOSE_MM    VERBOSE || 0
#define VERBOSE_PARSE VERBOSE || 0
#define VERBOSE_EVAL  VERBOSE || 0

// reference count increment functions check for overflow,
// and keep the object's count pinned at the max value.
// this constant controls whether this event gets logged.
static const bool report_pinned_counts = true;

// all types are upper case by convention.
typedef char Char;
typedef unsigned char Byte;
typedef long Int;
typedef unsigned long Uns;
typedef bool Bool;

typedef int8_t    I8;
typedef uint8_t   U8;
typedef int16_t   I16;
typedef uint16_t  U16;
typedef int32_t   I32;
typedef uint32_t  U32;
typedef int64_t   I64;
typedef uint64_t  U64;
typedef float     F32;
typedef double    F64;

typedef void* Ptr;

typedef union {
  I32 i;
  U32 u;
  F32 f;
} W32; // 32 bit generic word.

typedef union {
  I64 i;
  U64 u;
  F64 f;
} W64; // 64 bit generic word.


#if __SIZEOF_POINTER__ == 4
#define ARCH_32_WORD 1
#define ARCH_64_WORD 0
#define width_word 32
typedef float Flt;

#elif __SIZEOF_POINTER__ == 8
#define ARCH_32_WORD 0
#define ARCH_64_WORD 1
#define width_word 64
typedef double Flt;

#else
// could also define ARCH_32_WIDE, which would use long long values.
// this would require additional checks.
#error "unknown architecture"
#endif

typedef union {
  Int i;
  Uns u;
  Flt f;
  Ptr p;
} Word; // generic word.

// enforce usage of custom types defined above.
#undef bool
// macros for all the standard types that will expand into parse errors.
#define char      / / /
#define long      / / /
#define unsigned  / / /
#define int8_t    / / /
#define uint8_t   / / /
#define int16_t   / / /
#define uint16_t  / / /
#define int32_t   / / /
#define uint32_t  / / /
#define int64_t   / / /
#define uint64_t  / / /
#define float     / / /
#define double    / / /

// integer range constants.
static const Int min_Int = LONG_MIN;
static const Int max_Int = LONG_MAX;
static const Uns max_Uns = ULONG_MAX;

// byte size constants.
// terminology: size_ prefix should always refer to size in bytes.
// define signed Int constants to use instead of sizeof.
// this lets us use signed Int with fewer casts, reducing the risk of overflows mistakes.
#define DEF_SIZE(type) static const Int size_##type = sizeof(type)

DEF_SIZE(Char);
DEF_SIZE(Bool);
DEF_SIZE(Int);
DEF_SIZE(Uns);
DEF_SIZE(Flt);
DEF_SIZE(Ptr);
DEF_SIZE(Word);

// this alignment value is based on the OSX malloc man page,
// which guarantees 16-byte-aligned malloc.
// TODO: values for other platforms.
// terminology: width_ prefix should always refer to a number of bits.
static const Uns width_min_alloc = 4; // bits
static const Int size_min_alloc = 1 << width_min_alloc;


static void assert_host_basic() {
  // a few sanity checks; called by main.
  assert(size_Word == size_Int);
  assert(size_Word == size_Uns);
  assert(size_Word == size_Flt);
  assert(size_Word == size_Ptr);
  assert(sizeof(wchar_t) == 4);
}


#define len_buffer 256 // standard buffer size for various Char arrays on the stack.


// looping macros

#define loop while (1) // infinite loop

// for Int 'i' from 'm' to 'n' in increments of 's'.
#define for_imns(i, m, n, s) \
for (Int i = (m), _##i##_end = (n), _##i##_step = (s); i < _##i##_end; i += _##i##_step)

// produces the same values for i as above, but in reverse order.
#define for_imns_rev(i, m, n, s) \
for (Int i = (n) - 1, _##i##_end = (m), _##i##_step = (s); i >= _##i##_end; i -= _##i##_step)

// same as for_imns(i, m, n, 0).
#define for_imn(i, m, n)      for_imns(i, (m), (n), 1)
#define for_imn_rev(i, m, n)  for_imns_rev(i, (m), (n), 1)

// same as for_imn(i, 0, n).
#define for_in(i, n)      for_imns(i, 0, (n), 1)
#define for_in_rev(i, n)  for_imns_rev(i, 0, (n), 1)

// returns -1, 0, or 1 based on sign of input.
#define sign(x) ({__typeof__(x) __x = (x); __x > 0 ? 1 : (__x < 0 ? -1 : 0); })

// use the cast macro to make all casts easily searchable for audits.
#define cast(t, ...) (t)(__VA_ARGS__)

// used to create switch statements that return strings for enum names.
#define CASE_RET_TOK(t) case t: return #t
#define CASE_RET_TOK_SPLIT(prefix, t) case prefix##t: return #t

// boolean logic
#define bit(x) (!!(x))
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

#if   ARCH_32_WORD
#define ALIGNED_TO_WORD ALIGNED_TO_4
#elif ARCH_64_WORD
#define ALIGNED_TO_WORD ALIGNED_TO_8
#endif

// mark a function as having no side effects.
#define PURE __attribute__((pure))

// const is like pure, but also does not access any values except the arguments;
// dereferencing pointer arguments is disallowed.
#define PURE_VAL __attribute__((const))

// clang c function overloading extension.
#define OVERLOAD __attribute__((overloadable))


UNUSED_FN static Word word_with_Int(Int i) { return (Word){.i=i}; }
UNUSED_FN static Word word_with_Uns(Uns u) { return (Word){.u=u}; }
UNUSED_FN static Word word_with_Flt(Flt f) { return (Word){.f=f}; }
UNUSED_FN static Word word_with_Ptr(Ptr p) { return (Word){.p=p}; }


static Int int_min(Int a, Int b) {
  return (a < b) ? a : b;
}

static Int int_max(Int a, Int b) {
  return (a > b) ? a : b;
}

static Int int_clamp(Int x, Int a, Int b) {
  return int_max(int_min(x, b), a);
}

