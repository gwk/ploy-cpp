// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference object functions.

#include "09-rc.h"


typedef enum {
  rt_mutable_bit = 1 << 0,
} Ref_tag;


struct _Ref_head {
  Obj tagged_type;
};


static Obj ref_type(Obj r) {
  assert_valid_ref(r);
  Obj t = r.h->tagged_type;
  t.u &= ~cast(Uns, rt_mutable_bit); // mask off the mutable bit.
  return t;
}


static Bool ref_is_mutable(Obj r) {
  assert_valid_ref(r);
  Obj type = r.h->tagged_type;
  return type.u & rt_mutable_bit; // get the mutable bit.
}


UNUSED_FN static void ref_freeze(Obj r) {
  assert_valid_ref(r);
  r.h->tagged_type.u &= ~cast(Uns, rt_mutable_bit); // unset the mutable bit.
}


static Bool ref_is_data(Obj d) {
  return is(ref_type(d), t_Data);
}


static Obj t_Env;

static Bool ref_is_env(Obj r) {
  return is(ref_type(r), t_Env);
}


static Bool ref_is_struct(Obj r) {
  return !ref_is_data(r) && !ref_is_env(r);
}


static Obj t_Type;

static Bool ref_is_type(Obj r) {
  return is(ref_type(r), t_Type);
}


static Obj ref_alloc(Int size) {
  assert(size >= size_Word * 2);
  counter_inc(ci_Ref_rc); // ret/rel counter.
  Obj r = (Obj){.r=raw_alloc(size, ci_Ref_alloc)}; // alloc counter also incremented.
  assert(!obj_tag(r)); // check that alloc is sufficiently aligned for pointer tagging.
  rc_insert(r);
  return r;
}


static Obj ref_init(Obj r, Obj type, Bool is_mutable) {
  // owns type,
  assert_valid_ref(r);
  r.h->tagged_type = type;
  if (is_mutable) {
    r.h->tagged_type.u |= rt_mutable_bit; // set the mutable bit.
  }
  return r;
}


static Obj ref_new(Int size, Obj type, Bool is_mutable) {
  // owns type.
  return ref_init(ref_alloc(size), type, is_mutable);
}


static Obj env_rel_fields(Obj o);
static Obj struct_rel_fields(Obj s);

static Obj ref_dealloc(Obj r) {
  //errFL("DEALLOC: %p:%o", r, r);
  rc_rel(ref_type(r));
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
