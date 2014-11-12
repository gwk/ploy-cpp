// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// imports and macros.

#ifndef OPT
#error "must define OPT to either 0 (debug) or 1 (optimized)"
#endif

#if OPT
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

// enable tail-call optimizations.
#ifndef OPTION_TCO
#define OPTION_TCO 1
#endif

#ifndef OPTION_REC_LIMIT
#define OPTION_REC_LIMIT (1<<10)
#endif

// zero object words for new, moved, and released elements to catch invalid accesses.
#ifndef OPTION_MEM_ZERO
#define OPTION_MEM_ZERO !OPT
#endif

// on ref object dealloc, do not actually call free to aid in debugging overelease bugs.
#ifndef OPTION_DEALLOC_PRESERVE
#define OPTION_DEALLOC_PRESERVE !OPT
#endif

// count all heap allocations and deallocations.
#ifndef OPTION_ALLOC_COUNT
#define OPTION_ALLOC_COUNT !OPT
#endif

#ifndef OPTION_BLANK_PTR_REPR
#define OPTION_BLANK_PTR_REPR 0
#endif

// verbose logging to aid debugging.
#define VERBOSE_PARSE 0

// disallow use of NULL in favor of nullptr, which we shorten to null.
#undef NULL
#define null nullptr

// wherever possible, we use types that are the natural word size.
#if __SIZEOF_POINTER__ == 4
#define ARCH_32_WORD 1
#define ARCH_64_WORD 0
#define size_Word 4
#define width_Word 32
#elif __SIZEOF_POINTER__ == 8
#define ARCH_32_WORD 0
#define ARCH_64_WORD 1
#define size_Word 8
#define width_Word 64
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

// equivalent to for_imns(i, m, n, 1).
#define for_imn(i, m, n)      for_imns(i, (m), (n), 1)
#define for_imn_rev(i, m, n)  for_imns_rev(i, (m), (n), 1)

// equivalent to for_imns(i, 0, n, s).
#define for_ins(i, n, s)      for_imns(i, 0, (n), (s))
#define for_ins_rev(i, n, s)  for_imns_rev(i, 0, (n), (s))

// equivalent to for_imns(i, 0, n, 1).
#define for_in(i, n)      for_imns(i, 0, (n), 1)
#define for_in_rev(i, n)  for_imns_rev(i, 0, (n), 1)

// returns -1, 0, or 1 based on sign of input.
#define sign(x) ({__typeof__(x) __x = (x); __x > 0 ? 1 : (__x < 0 ? -1 : 0); })

// used to create switch statements that return strings for enum names.
#define CASE_RET_TOK(t) case t: return #t
#define CASE_RET_TOK_SPLIT(prefix, t) case prefix##t: return #t

// boolean logic
#define bit(x) (!!(x)) // 0 for falsy, 1 for truthy.
#define XOR(a, b) (bit(a) ^ bit(b)) // logical exclusive-or.

// force struct alignment. ex: struct S { I32 a, b; } ALIGNED_TO_8;
#define ALIGNED_TO_4 __attribute__((__aligned__(4)))
#define ALIGNED_TO_8 __attribute__((__aligned__(8)))

#if   ARCH_32_WORD
#define ALIGNED_TO_WORD ALIGNED_TO_4
#elif ARCH_64_WORD
#define ALIGNED_TO_WORD ALIGNED_TO_8
#endif

// suppress unused warnings. ex: UNUSED f(UNUSED Int x) {...}
#define UNUSED __attribute__((unused))

// suppress unused var warnings; useful for vars defined within a macro expansion.
#define STRING_FROM_TOKEN(x) #x
#define UNUSED_VAR(x) _Pragma(STRING_FROM_TOKEN(unused(x)))


// mark a function as having no side effects.
#define PURE __attribute__((pure))

// const is like pure, but also does not access any values except the arguments;
// dereferencing pointer arguments is disallowed.
#define PURE_VAL __attribute__((const))

#if OPT
#define DBG UNUSED
#else
#define DBG
#endif

// no difference in 'extra' assertions for now, but allow annotating them as such.
#define assert_extra(condition) assert(condition)
