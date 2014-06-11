// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// simple word-size types and associated functions.

#include "01-macros.h"


#define len_buffer 256 // standard buffer size for various Char arrays on the stack.
// TODO: why can't this be a const Int?

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
typedef float Flt;
#elif __SIZEOF_POINTER__ == 8
typedef double Flt;
#else
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

typedef FILE* CFile; // 'File' refers to the ploy type.

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
DEF_SIZE(CFile);

// the assumed heap pointer alignment is based on the OSX malloc man page,
// which guarantees 16-byte-aligned malloc.
// TODO: values for other platforms.
// terminology: width_ prefix should always refer to a number of bits.
static const Uns width_min_alloc = 4; // in bits.
static const Int size_min_alloc = 1 << width_min_alloc;


static void assert_host_basic() {
  // a few sanity checks; called by main.
  assert(size_Word == size_Int);
  assert(size_Word == size_Uns);
  assert(size_Word == size_Flt);
  assert(size_Word == size_Ptr);
  assert(sizeof(wchar_t) == 4);
}


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
