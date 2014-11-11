// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "21-cmpd.h"

// flat array of unit type, singleton interleaved pairs.
static Array globals = array0;


static void global_push(Obj global) {
  array_append(&globals, rc_ret(global));
}


#if OPTION_ALLOC_COUNT
static void global_cleanup() {
  globals.mem.dissolve_dealloc();
}
#endif
