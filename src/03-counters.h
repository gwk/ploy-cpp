// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// memory allocation counters.

#include "02-word.h"


// counters enum names, structured as an x macro list.
// the C macro is temporarily defined to generate various lists of expressions.
// note: starting with 'Data', the order must match that of Struct_tag.
#define COUNTER_LIST \
C(Chars) \
C(Str) \
C(Mem) \
C(Hash_bucket) \
C(Set) \
C(Int) \
C(Sym) \
C(Data_word) \
C(Data) \
C(Data_allocs) \
C(Vec) \
C(Vec_allocs) \
C(I32) \
C(I32_allocs) \
C(I64) \
C(I64_allocs) \
C(U32) \
C(U32_allocs) \
C(U64) \
C(U64_allocs) \
C(F32) \
C(F32_allocs) \
C(F64) \
C(F64_allocs) \
C(File) \
C(File_allocs) \
C(Func_host_1) \
C(Func_host_1_allocs) \
C(Func_host_2) \
C(Func_host_2_allocs) \
C(Func_host_3) \
C(Func_host_3_allocs)

// index enum for the counters.
typedef enum {
#define C(c) ci_##c,
COUNTER_LIST
#undef C
  ci_end
} Counter_index;


#if OPT_ALLOC_COUNT

// the global array of inc/dec counter pairs.
static Int counters[ci_end][2] = {};


static void counter_inc(Counter_index ci) {
  assert(ci >= 0 && ci < ci_end);
  counters[ci][0]++;
}


static void counter_dec(Counter_index ci) {
  counters[ci][1]++;
}


static void counter_stats(Bool log_all) {
  // log counter stats.
#define C(c) { \
  Int inc = counters[ci_##c][0]; \
  Int dec = counters[ci_##c][1]; \
  const Char* name = #c; \
  if (log_all || inc != dec) { \
    fprintf(stderr, "==== PLOY MEMORY STATS: %s: %ld - %ld = %ld\n", \
      name, inc, dec, inc - dec); \
  } \
}
COUNTER_LIST
#undef C
}

#else // !OPT_ALLOC_COUNT

#define counter_inc(ci) ((void)0)
#define counter_dec(ci) ((void)0)
#define counter_stats(ci) ((void)0)

#endif // OPT_ALLOC_COUNT
