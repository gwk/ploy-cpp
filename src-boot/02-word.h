// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// simple word-size types and associated functions.

#include "01-macros.h"


#define len_buffer 256 // standard buffer size for various Char arrays on the stack.
// this cannot be a const Int due to c limitations.

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

typedef void* Raw;

typedef Char* CharsM;
typedef const Char* Chars;

#if ARCH_32_WORD
typedef float Flt;
#elif ARCH_64_WORD
typedef double Flt;
#endif

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
static const I32 min_I32 = INT_MIN;
static const I32 max_I32 = INT_MAX;
static const U32 max_U32 = UINT_MAX;

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
DEF_SIZE(Raw);
DEF_SIZE(CFile);

// the assumed heap pointer alignment is based on the OSX malloc man page,
// which guarantees 16-byte-aligned malloc.
// TODO: values for other platforms.
// terminology: width_* should always refer to a number of bits.
static const Uns width_min_alloc = 4; // in mask bits.
static const Int size_min_alloc = 1 << width_min_alloc; // in bytes.


// stderr utilities: write info/error/debug messages to the console.

#define err(s) fputs((s), stderr)
#define err_nl() err("\n")
#define err_flush() fflush(stderr)
#define errL(s) { err(s); err_nl(); }

static void fmt_to_file(CFile f, Chars fmt, ...);

// note: the final token pasting is between the comma and __VA_ARGS__.
// this is a gnu extension to elide the comma in the case where __VA_ARGS__ is empty.
#define errF(fmt, ...) fmt_to_file(stderr, fmt, ## __VA_ARGS__)
#define errFL(fmt, ...) errF(fmt "\n", ## __VA_ARGS__)


// write an error message and then exit.
#define error(fmt, ...) { \
  errFL("ploy error: " fmt, ## __VA_ARGS__); \
  fail(); \
}

union Obj;
struct Trace;

static NO_RETURN _exc_raise(Trace* trace, Obj env, Chars fmt, ...);

// check that a condition is true; otherwise error.
#define check(condition, fmt, ...) { if (!(condition)) error(fmt, ## __VA_ARGS__); }

// NOTE: the exc macros expect env to be defined in the current scope.
#define exc_raise(fmt, ...) _exc_raise(t, env, fmt, ##__VA_ARGS__)
#define exc_check(condition, ...) if (!(condition)) exc_raise(__VA_ARGS__)


NO_RETURN fail(void);
NO_RETURN fail() {
  exit(1);
}


static void assert_host_basic() {
  // a few sanity checks; called by main.
  assert(size_Raw == size_Int);
  assert(size_Raw == size_Uns);
  assert(size_Raw == size_Raw);
}


static Int int_min(Int a, Int b) {
  return (a < b) ? a : b;
}

static Int int_max(Int a, Int b) {
  return (a > b) ? a : b;
}

static Int int_clamp(Int x, Int a, Int b) {
  return int_max(int_min(x, b), a);
}


#if OPTION_RC_TABLE_STATS
static Int int_pow2_fl(Int x) {
  if (x < 0) x *= -1;
  Int p = -1;
  while (x) {
    p++;
    x >>= 1;
  }
  return p;
}
#endif
