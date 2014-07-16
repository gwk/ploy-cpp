// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference object functions.

#include "09-rc.h"


static const Obj s_Env;
static Obj t_Type;


static Obj ref_type(Obj r) {
  assert(obj_is_ref(r));
  return *r.type_ptr;
}


static Bool ref_is_data(Obj d) {
  return is(ref_type(d), s_Data);
}


static Bool ref_is_env(Obj r) {
  return is(ref_type(r), s_Env);
}


static Bool ref_is_struct(Obj r) {
  return !ref_is_data(r) && !ref_is_env(r);
}


static Bool ref_is_type(Obj r) {
  return is(ref_type(r), t_Type);
}


static Obj ref_alloc(Int size) {
  assert(size >= size_Word * 2);
  counter_inc(ci_Ref_rc); // ret/rel counter.
  Obj r = (Obj){.r=raw_alloc(size, ci_Ref_alloc)}; // alloc counter also incremented.
  assert(!obj_tag(r)); // check that alloc is sufficiently aligned for pointer tagging.
  rc_insert(r);
  //errFL("REF ALLOC: %d; total: %ld rc len: %ld", rt, counters[ci_alloc][0] - counters[ci_alloc][1], rc_table.len);
  return r;
}


static Obj env_rel_fields(Obj o);
static Obj struct_rel_fields(Obj s);

static Obj ref_dealloc(Obj r) {
  rc_rel(*r.type_ptr);
  Obj tail;
  if (ref_is_data(r)) { // no extra action required.
    tail = obj0;
  } else if (ref_is_env(r)) {
    tail = env_rel_fields(r);
  } else {
    tail = struct_rel_fields(r);
  }
  // ret/rel counter has already been decremented by rc_rel.
#if !OPT_DEALLOC_PRESERVE
  raw_dealloc(r.r, ci_Ref_alloc);
#elif OPT_ALLOC_COUNT
  // manually count for the missing raw_dealloc.
  counter_dec(ci_Ref_alloc);
#endif
  return tail;
}
