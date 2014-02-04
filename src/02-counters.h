// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "01-word.h"


#define width_obj_tag 3
#define obj_tag_end (1L << width_obj_tag)

#define width_struct_tag 4
#define struct_tag_end (1L << width_struct_tag)


#if OPT_ALLOC_COUNT
static Int total_allocs_raw[2] = {};
static Int total_allocs_mem[2] = {};
#endif


static Int total_rets[obj_tag_end][2] = {};
static Int total_allocs_ref[struct_tag_end][2] = {};
