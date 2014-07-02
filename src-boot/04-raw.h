// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// heap allocation functions.
// for the sake of accurate counting, no other code should call calloc/malloc/realloc;
// the only legitimate uses of free are to match local asprintf and vasprintf.

#include "03-counters.h"


static Raw raw_alloc(Int size, Counter_index ci) {
  assert(size >= 0);
  // malloc does not return NULL for zero size;
  // we do so that a len/ptr pair with zero len always has a NULL ptr.
  if (!size) return NULL;
  counter_inc(ci); // do not count NULL.
  Raw p = malloc(cast(Uns, size));
  if (!p) {
    fprintf(stderr, "raw_alloc failed; size: %ld", size);
    exit(1);
  }
#if OPT_ALLOC_SCRIBBLE
  memset(p, 0xAA, size); // same value as OSX MallocPreScribble.
#endif
  return p;
}


static void raw_dealloc(Raw p, Counter_index ci) {
  if (p) { // do not count NULL.
    counter_dec(ci);
    free(p);
  }
}


static Raw raw_realloc(Raw p, Int size, Counter_index ci) {
  assert(size >= 0);
  if (!p) { // old size is 0, so treate this as an initial alloc.
    return raw_alloc(size, ci);
  }
  if (size) {
    Raw r = realloc(p, cast(Uns, size));
    if (!r) {
      fprintf(stderr, "raw_realloc failed; size: %ld", size);
      exit(1);
    }
    return r;
  } else {
    raw_dealloc(p, ci); // realloc does not return NULL for zero size; see note in raw_alloc.
    return NULL;
  }
}
