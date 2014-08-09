// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// imports and macros.

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

// include the generated source here, before 'char' is disallowed via macros below.
#include "ploy-core.h"

// on ref object dealloc, do not actually call free to aid in debugging overelease bugs.
#ifndef OPT_DEALLOC_PRESERVE
#define OPT_DEALLOC_PRESERVE DEBUG
#endif

// count all heap allocations and deallocations.
#ifndef OPT_ALLOC_COUNT
#define OPT_ALLOC_COUNT DEBUG
#endif

// reference counter hash table stats.
#ifndef OPT_RC_TABLE_STATS
#define OPT_RC_TABLE_STATS 0
#endif

// write invalid or unlikely values into unused portions of heap allocations.
// this helps detect illegal reads from uninitialized and dead regions,
// and also helps detect errors in allocation code,
// because we immediately attempt to write to the entire allocation.
// it is not totally redundant with the MallocScribble and MallocPreScribble
// environment variables on OSX, but it is completely compatible.
#ifndef OPT_ALLOC_SCRIBBLE
#define OPT_ALLOC_SCRIBBLE DEBUG
#endif

// verbose logging to aid in various debugging scenarios.
#define VERBOSE 0
#define VERBOSE_PARSE   VERBOSE || 0

// wherever possible, we use types that are the natural word size.
#if __SIZEOF_POINTER__ == 4
#define ARCH_32_WORD 1
#define ARCH_64_WORD 0
#define width_word 32
#elif __SIZEOF_POINTER__ == 8
#define ARCH_32_WORD 0
#define ARCH_64_WORD 1
#define width_word 64
#else
#error "unknown architecture"
#endif

// looping macros

#define loop while (1) // infinite loop

// for Int 'i' from 'm' to 'n' in increments of 's'.
#define for_imns(i, m, n, s) \
for (Int i = (m), _end_##i = (n), _step_##i = (s); i < _end_##i; i += _step_##i)

// produces the same values for i as above, but in reverse order.
#define for_imns_rev(i, m, n, s) \
for (Int i = (n) - 1, _end_##i = (m), _step_##i = (s); i >= _end_##i; i -= _step_##i)

// equivalent to for_imns(i, m, n, 0).
#define for_imn(i, m, n)      for_imns(i, (m), (n), 1)
#define for_imn_rev(i, m, n)  for_imns_rev(i, (m), (n), 1)

// equivalent to for_imn(i, 0, n).
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
#define bit(x) (!!(x)) // 0 for falsy, 1 for truthy.
#define XOR(a, b) (bit(a) ^ bit(b)) // logical exclusive-or.

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

// mark function as never returning. ex: NO_RETURN f() {...}
#define NO_RETURN __attribute__((noreturn)) void

// suppress compiler warnings. ex: UNUSED_FN f() {...}
#define UNUSED_FN __attribute__((unused))

// mark a function as having no side effects.
#define PURE __attribute__((pure))

// const is like pure, but also does not access any values except the arguments;
// dereferencing pointer arguments is disallowed.
#define PURE_VAL __attribute__((const))

// clang c function overloading extension.
#define OVERLOAD __attribute__((overloadable))


#if DEBUG
#define DEBUG_FN
#else
#define DEBUG_FN UNUSED_FN
#endif
