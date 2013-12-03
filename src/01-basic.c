// Copyright 2011 George King.
// Permission to use this file is granted in ploy/license.txt.


// exclude the standard libraries when preprocessing the source for brevity (see preprocess.sh)
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
#endif


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

