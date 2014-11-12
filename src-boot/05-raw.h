// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// heap allocation functions.
// for the sake of accurate counting, no other code should call calloc/malloc/realloc;
// the only legitimate uses of free are to match local asprintf and vasprintf.

#include "04-counters.h"


static Raw raw_alloc(Int size, DBG Counter_index ci) {
  assert(size >= 0);
  // malloc does not return null for zero size;
  // we do so that a len/ptr pair with zero len always has a null ptr.
  if (!size) return null;
  counter_inc(ci); // do not count null.
  Raw p = malloc(Uns(size));
  if (!p) {
    fprintf(stderr, "raw_alloc failed; size: %ld", size);
    fail();
  }
  return p;
}


static void raw_dealloc(Raw p, DBG Counter_index ci) {
  if (p) { // do not count null.
    counter_dec(ci);
    free(p);
  }
}


static Raw raw_realloc(Raw p, Int size, Counter_index ci) {
  assert(size >= 0);
  if (!p) { // old size is 0, so treate this as an initial alloc for accounting.
    return raw_alloc(size, ci);
  }
  if (size) {
    Raw r = realloc(p, Uns(size));
    if (!r) {
      fprintf(stderr, "raw_realloc failed; size: %ld", size);
      fail();
    }
    return r;
  } else {
    raw_dealloc(p, ci); // realloc does not return null for zero size; see note in raw_alloc.
    return null;
  }
}
