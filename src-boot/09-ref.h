// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference object functions.

#include "08-rc.h"


static Int sym_index(Obj s);
static const Int sym_index_of_ref_type_sym_first;
static const Int sym_index_of_ref_type_sym_last;

static Ref_tag ref_tag(Obj r) {
  assert(obj_is_ref(r));
  Int si = sym_index(*r.type_ptr);
  assert(si >= sym_index_of_ref_type_sym_first && si <= sym_index_of_ref_type_sym_last);
  return cast(Ref_tag, si - sym_index_of_ref_type_sym_first);
}


static Obj ref_type_sym(Obj r) {
  assert(obj_is_ref(r));
  return *r.type_ptr;
}


static Counter_index rt_counter_index(Ref_tag rt) {
  // note: this math relies on the layout of both COUNTER_LIST and Struct_tag.
  return ci_Data + (rt * 2);
}


#if OPT_ALLOC_COUNT
static Counter_index ref_counter_index(Obj r) {
  assert(obj_is_ref(r));
  return rt_counter_index(ref_tag(r));
}
#endif


static Bool ref_is_data(Obj d) {
  return ref_tag(d) == rt_Data;
}


static Bool ref_is_vec(Obj r) {
  return ref_tag(r) == rt_Vec;
}


static Bool ref_is_env(Obj r) {
  return ref_tag(r) == rt_Env;
}


static Bool ref_is_file(Obj f) {
  return ref_tag(f) == rt_File;
}


static Obj ref_alloc(Ref_tag rt, Int size) {
  assert(size >= size_Word * 2);
  Counter_index ci = rt_counter_index(rt);
  Counter_index ci_alloc = ci + 1; // this math relies on the layout of COUNTER_LIST.
  counter_inc(ci); // ret/rel counter.
  Obj r = (Obj){.p=raw_alloc(size, ci_alloc)}; // alloc counter also incremented.
  assert(!obj_tag(r)); // check that alloc is really aligned to allow tagging.
  rc_insert(r);
  //errFL("REF ALLOC: %d; total: %ld rc len: %ld", rt, counters[ci_alloc][0] - counters[ci_alloc][1], rc_table.len);
  return r;
}


static void env_rel_fields(Obj o);

static void ref_dealloc(Obj r) {
  Ref_tag rt = ref_tag(r);
  if (rt == rt_Vec) {
    it_vec_ref(it, r) {
      // TODO: make this tail recursive for deallocating long chains?
      rc_rel(*it);
    }
  } else if (rt == rt_Env) {
    env_rel_fields(r);
  }
  // ret/rel counter has already been decremented by rc_rel.
#if !OPT_DEALLOC_PRESERVE
  raw_dealloc(r.p, rt_counter_index(rt) + 1); // math relies on layout of COUNTER_LIST.
#elif OPT_ALLOC_COUNT
  // manually count for the missing raw_dealloc.
  counter_dec(rt_counter_index(rt) + 1); // math relies on layout of COUNTER_LIST.
#endif
}
