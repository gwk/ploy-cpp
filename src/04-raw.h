// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "03-bytes.h"


static Ptr raw_alloc(Int size) {
  assert(size >= 0);
#if OPT_ALLOC_COUNT
  if (size > 0) {
    total_allocs_raw[0]++;
  }
#endif
  return malloc(cast(Uns, size));
}


static void raw_dealloc(Ptr p) {
#if OPT_ALLOC_COUNT
  if (p) {
    total_allocs_raw[1]++;
  }
#endif
  free(p);
}

