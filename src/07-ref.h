// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "06-obj.h"


static Uns ref_hash(Obj r) {
  assert(obj_is_ref(r));
  return r.u >> width_min_malloc;
}


static void assert_ref_is_valid(Obj r) {
  assert(obj_is_ref(r));
  assert(r.rc->sc > 0);
}


static Struct_tag ref_struct_tag(Obj r) {
  assert_ref_is_valid(r);
  return r.rc->st;
}


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


static Int ref_data_len(Obj r) {
  assert(ref_is_data(r));
  assert(r.rcl->len > 0);
  return r.rcl->len;
}


static B ref_data_ptr(Obj d) {
  assert(ref_is_data(d));
  return (B){.m = cast(BM, d.rcl + 1)}; // address past rcl.
}


static Obj ref_alloc(Struct_tag st, Int width) {
  assert(width > 0);
  Obj r = (Obj){.p=malloc(cast(Uns, width))};
  assert(!obj_tag(r)); // check that malloc is really aligned to the width of the tag.
  r.rc->st = st;
  r.rc->wc = 0;
  r.rc->mt = 0;
  r.rc->sc = 1;
  rc_errMLV("alloc     ", r.rc);
#if OPT_ALLOC_COUNT
  Counter_index ci = obj_counter_index(r);
  counter_inc(ci); // ret counter.
  counter_inc(ci + 1); // alloc counter.
#endif
  return r;
}


static Int vec_len(Obj v);
static Obj* vec_els(Obj v);

static void ref_dealloc(Obj r) {
  assert_ref_is_valid(r);
  rc_errMLV("dealloc   ", r.rc);
  // TODO: could choose to pin the strong and weak counts and leak the object.
  check(r.rc->wc == 0, "attempt to deallocate object with non-zero weak count: %p", r.p);
  Struct_tag st = ref_struct_tag(r);
  if (st == st_Vec) {
    Obj* els = vec_els(r);
    for_in(i, vec_len(r)) {
      //err("  el rel: "); dbg(els[i]);
      // TODO: make this tail recursive for deallocating long chains?
      obj_rel(els[i]);
    }
  }
#if OPT_DEALLOC_MARK
  assert(r.rc->sc == 1);
  r.rc->sc = 0;
#endif
#if !OPT_DEALLOC_PRESERVE
  free(r.p);
#endif
#if OPT_ALLOC_COUNT
  Counter_index ci = obj_counter_index(r);
  counter_dec(ci + 1); // alloc counter.
#endif
}

