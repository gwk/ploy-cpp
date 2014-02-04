// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "02-bytes.h"

#define COUNTER_LIST \
C(Bytes) \
C(Str) \
C(Mem) \
C(Hash_bucket) \
C(Set) \
C(Flt) \
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


typedef enum {
#define C(c) ci_##c,
COUNTER_LIST
#undef C
  ci_end
} Counter_index;



#if OPT_ALLOC_COUNT
static Int counters[ci_end][2] = {};
#endif

static void counter_inc(Counter_index ci) {
  assert(ci >= 0 && ci < ci_end);
#if OPT_ALLOC_COUNT
  counters[ci][0]++;
#endif
}


static void counter_dec(Counter_index ci) {
#if OPT_ALLOC_COUNT
  counters[ci][1]++;
#endif
}


#if OPT_ALLOC_COUNT
static void counter_stats(Bool verbose) {

#define C(c) { \
  Int inc = counters[ci_##c][0]; \
  Int dec = counters[ci_##c][1]; \
  BC name = #c; \
  if (verbose || inc != dec) { \
    errFL("==== PLOY MEMORY STATS: %s: %ld - %ld = %ld", name, inc, dec, inc - dec); \
  } \
}
COUNTER_LIST
#undef C
}
#endif // OPT_ALLOC_COUNT

