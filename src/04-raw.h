// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "03-counters.h"


static Ptr raw_alloc(Int size, Counter_index ci) {
  assert(size >= 0);
  if (!size) return NULL;
  counter_inc(ci);
  return malloc(cast(Uns, size));
}


static void raw_dealloc(Ptr p, Counter_index ci) {
  if (p) {
    counter_dec(ci);
  }
  free(p);
}


static Ptr raw_realloc(Ptr p, Int size, Counter_index ci) {
  assert(size >= 0);
  if (size) {
    if (!p) { // old size is 0
      counter_inc(ci);
    }
    Ptr q = realloc(p, cast(Uns, size));
    check(q, "realloc failed; size: %ld", size);
    return q;
  }
  else {
    raw_dealloc(p, ci); // realloc would return a non-null pointer, which is not what we want.
    return NULL;
  }
}

