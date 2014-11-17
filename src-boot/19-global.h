// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-parse.h"


static List globals;


static void global_push(Obj global) {
  globals.append(global.ret());
}


#if OPTION_ALLOC_COUNT
static void global_cleanup() {
  globals.dissolve_els_dealloc();
}
#endif
