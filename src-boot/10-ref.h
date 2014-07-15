// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference object functions.

#include "09-rc.h"


static Ref_tag ref_tag(Obj r);

static const Obj s_Env;


static Obj ref_type(Obj r) {
  assert(obj_is_ref(r));
  return *r.type_ptr;
}


static Counter_index rt_counter_index(Ref_tag rt) {
  // note: this math relies on the layout of both COUNTER_LIST and ref_tag.
  return ci_Data + (rt * 2);
}


#if OPT_ALLOC_COUNT
static Counter_index ref_counter_index(Obj r) {
  assert(obj_is_ref(r));
  return rt_counter_index(ref_tag(r));
}
#endif


static Bool ref_is_data(Obj d) {
  return is(ref_type(d), s_Data);
}


static Bool ref_is_env(Obj r) {
  return is(ref_type(r), s_Env);
}


static Bool ref_is_struct(Obj r) {
  return !ref_is_data(r) && !ref_is_env(r);
}


static Obj ref_alloc(Ref_tag rt, Int size) {
  assert(size >= size_Word * 2);
  Counter_index ci = rt_counter_index(rt);
  Counter_index ci_alloc = ci + 1; // this math relies on the layout of COUNTER_LIST.
  counter_inc(ci); // ret/rel counter.
  Obj r = (Obj){.r=raw_alloc(size, ci_alloc)}; // alloc counter also incremented.
  assert(!obj_tag(r)); // check that alloc is really aligned to allow tagging.
  rc_insert(r);
  //errFL("REF ALLOC: %d; total: %ld rc len: %ld", rt, counters[ci_alloc][0] - counters[ci_alloc][1], rc_table.len);
  return r;
}


static Obj env_rel_fields(Obj o);
static Obj struct_rel_fields(Obj s);

static Obj ref_dealloc(Obj r) {
  Ref_tag rt = ref_tag(r);
  rc_rel(*r.type_ptr);
  Obj tail = obj0;
  if (rt == rt_Struct) {
    tail = struct_rel_fields(r);
  } else if (rt == rt_Env) {
    tail = env_rel_fields(r);
  }
  // ret/rel counter has already been decremented by rc_rel.
#if !OPT_DEALLOC_PRESERVE
  raw_dealloc(r.r, rt_counter_index(rt) + 1); // math relies on layout of COUNTER_LIST.
#elif OPT_ALLOC_COUNT
  // manually count for the missing raw_dealloc.
  counter_dec(rt_counter_index(rt) + 1); // math relies on layout of COUNTER_LIST.
#endif
  return tail;
}
