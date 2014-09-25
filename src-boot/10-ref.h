// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference object functions.

#include "09-rc.h"


struct _Ref_head {
  Obj type;
};


static Obj ref_type(Obj r) {
  assert_valid_ref(r);
  return r.h->type;
}


static Bool ref_is_data(Obj d) {
  return is(ref_type(d), t_Data);
}


static Obj t_Env;

static Bool ref_is_env(Obj r) {
  return is(ref_type(r), t_Env);
}


static Bool ref_is_cmpd(Obj r) {
  return !ref_is_data(r) && !ref_is_env(r);
}


static Obj t_Type;

static Bool ref_is_type(Obj r) {
  return is(ref_type(r), t_Type);
}


static Obj ref_new(Int size, Obj type) {
  assert(size >= size_Word * 2);
  counter_inc(ci_Ref_rc); // ret/rel counter.
  Obj r = (Obj){.r=raw_alloc(size, ci_Ref_alloc)}; // alloc counter also incremented.
  assert(!obj_tag(r)); // check that alloc is sufficiently aligned for pointer tagging.
  rc_insert(r);
  r.h->type = type;
  return r;
}


static Obj env_rel_fields(Obj o);
static Obj cmpd_rel_fields(Obj c);

static Obj ref_dealloc(Obj r) {
  //errFL("DEALLOC: %p:%o", r, r);
  rc_rel(ref_type(r));
  Obj tail;
  if (ref_is_data(r)) { // no extra action required.
    tail = obj0;
  } else if (ref_is_env(r)) {
    tail = env_rel_fields(r);
  } else {
    tail = cmpd_rel_fields(r);
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
