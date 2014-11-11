// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference object functions.

#include "09-rc.h"


static Obj ref_type(Obj r) {
  assert(r.is_ref());
  return r.h->type;
}


static Bool ref_is_data(Obj d) {
  return ref_type(d) == t_Data;
}


extern Obj t_Env;

static Bool ref_is_env(Obj r) {
  return ref_type(r) == t_Env;
}


static Bool ref_is_cmpd(Obj r) {
  return !ref_is_data(r) && !ref_is_env(r);
}


extern Obj t_Type;

static Bool ref_is_type(Obj r) {
  return ref_type(r) == t_Type;
}


static Obj ref_new(Int size, Obj type) {
  assert(size >= size_Raw * 2);
  counter_inc(ci_Ref_rc); // ret/rel counter.
  Obj r = Obj(raw_alloc(size, ci_Ref_alloc)); // alloc counter also incremented.
  assert(!r.tag()); // check that alloc is sufficiently aligned for pointer tagging.
  r.h->rc = (1<<1) + 1;
  r.h->type = type;
  return r;
}


static Obj env_rel_fields(Obj o);
static Obj cmpd_rel_fields(Obj c);

static Obj ref_dealloc(Obj r) {
  //errFL("DEALLOC: %p:%o", r.r, r);
  r.h->rc = 0;
  //assert(r != t_Type);
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
#if !OPTION_DEALLOC_PRESERVE
  raw_dealloc(r.r, ci_Ref_alloc);
#elif OPTION_ALLOC_COUNT
  // manually count for the missing raw_dealloc.
  counter_dec(ci_Ref_alloc);
#endif
  return tail;
}
