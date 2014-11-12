// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// upper-cased names for C types, and associated constants.

#include "01-defs.h"


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

// enforce usage of the above type names.
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

// size constants.
// terminology: size_* should always refer to size in bytes.
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
