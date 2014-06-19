// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference object functions.

#include "08-rc.h"


static Uns ref_hash(Obj r) {
  // pointer hash simply shifts of the bits that are guaranteed to be zero.
  assert(obj_is_ref(r));
  return r.u >> width_min_alloc;
}

  
static Struct_tag ref_struct_tag(Obj r) {
  assert(obj_is_ref(r));
  return r.rc->st;
}


#if OPT_ALLOC_COUNT

static Counter_index st_counter_index(Struct_tag st) {
  // note: this math relies on the layout of both COUNTER_LIST and Struct_tag.
  return ci_Data + (st * 2);
}


static Counter_index ref_counter_index(Obj r) {
  assert(obj_is_ref(r));
  return st_counter_index(ref_struct_tag(r));
}

#endif


static Bool ref_is_vec(Obj r) {
  return ref_struct_tag(r) == st_Vec;
}


static Bool ref_is_data(Obj d) {
  return ref_struct_tag(d) == st_Data;
}


static Bool ref_is_file(Obj f) {
  return ref_struct_tag(f) == st_File;
}


static Ptr ref_body(Obj r) {
  assert(!ref_is_data(r) && !ref_is_vec(r));
  return r.rc + 1; // address past rc.
}


static Obj ref_alloc(Struct_tag st, Int size) {
  assert(size >= size_Word * 2);
  Counter_index ci = st_counter_index(st);
  Counter_index ci_alloc = ci + 1; // this math relies on the layout of COUNTER_LIST.
  counter_inc(ci); // ret/rel counter.
  Obj r = (Obj){.p=raw_alloc(size, ci_alloc)}; // alloc counter also incremented.
  assert(!obj_tag(r)); // check that alloc is really aligned to allow tagging.
  r.rc->st = st;
  r.rc->sc = 1;
  rc_insert(ref_hash(r));
  return r;
}


static void ref_dealloc(Obj r) {
  Struct_tag st = ref_struct_tag(r);
  if (st == st_Vec) {
    it_vec_ref(it, r) {
      // TODO: make this tail recursive for deallocating long chains?
      rc_rel(*it);
    }
  }
  #if OPT_DEALLOC_MARK
  assert(r.rc->sc == 1);
  r.rc->sc = 0;
#endif
  // ret/rel counter has already been decremented by rc_rel.
#if !OPT_DEALLOC_PRESERVE
  raw_dealloc(r.p, st_counter_index(st) + 1); // math relies on layout of COUNTER_LIST.
#elif OPT_ALLOC_COUNT
  counter_dec(st_counter_index(st) + 1); // math relies on layout of COUNTER_LIST.
#endif
}
