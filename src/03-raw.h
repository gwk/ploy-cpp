// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// heap allocation functions.
// for the sake of accurate counting, no other code should call calloc/malloc/realloc;
// the only legitimate uses of free are to match local asprintf and vasprintf.

#include "02-counters.h"


static Ptr raw_alloc(Int size, Counter_index ci) {
  assert(size >= 0);
  if (!size) return NULL; // malloc does not return NULL for zero size; we do for clarity.
  counter_inc(ci); // do not count NULL.
  Ptr p = malloc(cast(Uns, size));
  if (!p) {
    fprintf(stderr, "raw_alloc failed; size: %ld", size);
    exit(1);
  }
#if OPT_ALLOC_SCRIBBLE
  memset(p, 0xAA, size); // same value as OSX MallocPreScribble.
#endif
  return p;
}


static void raw_dealloc(Ptr p, Counter_index ci) {
  if (p) { // do not count NULL.
    counter_dec(ci);
    free(p);
  }
}


static Ptr raw_realloc(Ptr p, Int size, Counter_index ci) {
  assert(size >= 0);
  if (!p) { // old size is 0, so treate this as an initial alloc.
    return raw_alloc(size, ci);
  }
  if (size) {
    Ptr r = realloc(p, cast(Uns, size));
    if (!r) {
      fprintf(stderr, "raw_realloc failed; size: %ld", size);
      exit(1);
    }
    return r;
  }
  else {
    raw_dealloc(p, ci); // realloc would return a non-null pointer; we do for clarity.
    return NULL;
  }
}
