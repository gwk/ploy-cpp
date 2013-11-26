// Copyright 2011 George King.
// Permission to use this file is granted in ploy/license.txt.


// exclude the standard libraries when preprocessing the source for brevity (see preprocess.sh)
#ifndef SKIP_LIB_INCLUDES
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <malloc/malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#endif

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

typedef const char* Ascii;
typedef const char* Utf8;
typedef const wchar_t* Utf32;

typedef char* AsciiM;
typedef char* Utf8M;
typedef wchar_t* Utf32M;

typedef FILE* File;

// integer range constants.
const Int min_Int = INT_MIN;
const Int max_Int = INT_MAX;
const Uns max_Uns = UINT_MAX;

// byte width constants.
const Uns width_Bool  = sizeof(Bool);
const Uns width_Int   = sizeof(Int);
const Uns width_Uns   = sizeof(Uns);
const Uns width_Ptr   = sizeof(void*);


#define assert_host_basic \
assert(sizeof(wchar_t) == 4)


#define loop while (1)
#define for_imns(i, m, n, s) for (Int i = (m), _##i##_end = (n), _##i##_step = (s); i < _##i##_end; i += _##i##_step)

#define for_imns_rev(i, m, n, s) \
for (Int i = (n) - 1, _##i##_end = (m), _##i##_step = (s); i >= _##i##_end; i -= _##i##_step)

#define for_imn(i, m, n) for_imns(i, (m), (n), 1)
#define for_in(i, n) for_imns(i, 0, (n), 1)

#define for_imn_rev(i, n) for_imns_rev(i, (m), (n), 1)
#define for_in_rev(i, n) for_imns_rev(i, 0, (n), 1)

#define sign(x) ({__typeof__(x) __x = (x); __x > 0 ? 1 : (__x < 0 ? -1 : 0); })


// used to create switch statements that return strings for enum names.
#define CASE_RET_TOK(t) case t: return #t
#define CASE_RET_TOK_SPLIT(prefix, t) case prefix##t: return #t

// iteration macros

// boolean logic
#define bit(x) ((x) ? true : false)
#define xor(a, b) (bit(a) ^ bit(b))

#define min(a, b) ({typeof(a) __a = (a); typeof(b) __b = (b); return (__b < __a) ? __b : __a; })
#define max(a, b) ({typeof(a) __a = (a); typeof(b) __b = (b); return (__b > __a) ? __b : __a; })


// type definition macros: define type T and standard raw constructor T_raw for field types and names (t0, n0, ...)

#define def_struct_2(T, t0, n0, t1, n1) \
typedef struct { t0 n0; t1 n1; } T; \
T raw_##T(t0 n0, t1 n1) { return (T){.n0=n0, .n1=n1}; }

#define def_struct_3(T, t0, n0, t1, n1, t2, n2) \
typedef struct { t0 n0; t1 n1; t2 n2; } T; \
T raw_##T(t0 n0, t1 n1, t2 n2) { return (T){.n0=n0, .n1=n1, .n2=n2}; }

#define def_struct_4(T, t0, n0, t1, n1, t2, n2, t3, n3) \
typedef struct { t0 n0; t1 n1; t2 n2; t3 n3; } T; \
T raw_##T(t0 n0, t1 n1, t2 n2, t3 n3) { (T){.n0=n0, .n1=n1, .n2=n2, .n3=n3}; }


// suppress compiler warnings.
// ex: UNUSED_FN f() {...}
#define UNUSED_FN __attribute__((unused))

// mark function as never returning.
// ex: NORETURN f() {...}
#define NORETURN __attribute__((noreturn)) void

// force struct alignment.
// ex: typedef struct { I32 a, b; } ALIGNED_8 S;
#define ALIGNED_TO_4 __attribute__((__aligned__(4)))
#define ALIGNED_TO_8 __attribute__((__aligned__(8)))

#if __SIZEOF_POINTER__ == 4
#define ALIGNED_TO_WORD ALIGNED_TO_4
#elif __SIZEOF_POINTER__ == 8
#define ALIGNED_TO_WORD ALIGNED_TO_8
#else
#error "unknown alignment"
#endif


// suppress unused var warnings.
#define QK_STRINGIFY(x) #x
#define UNUSED_VAR(x) _Pragma(QK_STRINGIFY(unused(x)))

