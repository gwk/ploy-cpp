// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// memory allocation counters.

#include "02-word.h"


// counters enum names, structured as an x macro list.
// the C macro is temporarily defined to generate various lists of expressions.
#define COUNTER_LIST \
C(Chars) \
C(RC_table) \
C(RC_bucket) \
C(Mem) \
C(Hash_bucket) \
C(Set) \
C(Dict) \
C(Type_table) \
C(Ptr_rc) \
C(Int_rc) \
C(Sym_rc) \
C(Data_word_rc) \
C(Ref_rc) \
C(Ref_alloc) \


// index enum for the counters.
enum Counter_index {
#define C(c) ci_##c,
COUNTER_LIST
#undef C
  ci_end
};


#if OPTION_ALLOC_COUNT

// the global array of inc/dec counter pairs.
static Int counters[ci_end][2] = {};

static Chars_const counter_names[] = {
#define C(c) #c,
COUNTER_LIST
#undef C
};


static void counter_inc(Counter_index ci) {
  // increment the specified counter.
  assert(ci >= 0 && ci < ci_end);
  counters[ci][0]++;
}


static void counter_dec(Counter_index ci) {
  // decrement the specified counter.
  counters[ci][1]++;
}


static void counter_stats(Bool log_all) {
  // log counter stats to stderr.
  for_in(i, ci_end) {
    Int inc = counters[i][0];
    Int dec = counters[i][1];
    if (log_all || inc != dec) {
      errFL("==== PLOY ALLOC STATS: %s: %i - %i = %i", counter_names[i], inc, dec, inc - dec);
    }
  }
}


#else // !OPTION_ALLOC_COUNT

#define counter_inc(ci) ((void)0)
#define counter_dec(ci) ((void)0)
#define counter_stats(ci) ((void)0)

#endif // OPTION_ALLOC_COUNT
