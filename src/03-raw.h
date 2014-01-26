// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "02-bytes.h"


static const Uns struct_tag_end;

#if OPT_ALLOC_COUNT
static Int total_allocs_raw = 0;
static Int total_allocs_mem = 0;
static Int total_deallocs_raw = 0;
static Int total_deallocs_mem = 0;
#endif


static Ptr raw_alloc(Int size) {
  assert(size >= 0);
#if OPT_ALLOC_COUNT
  if (size > 0) {
    total_allocs_raw++;
  }
#endif
  return malloc(cast(Uns, size));
}


static void raw_dealloc(Ptr p) {
#if OPT_ALLOC_COUNT
  if (p) {
    total_deallocs_raw++;
  }
#endif
  free(p);
}

