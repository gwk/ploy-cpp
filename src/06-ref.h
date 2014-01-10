// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "05-obj.h"


static void assert_ref_is_valid(Obj r) {
  assert(!obj_tag(r));
  assert(r.rc->st != st_DEALLOC);
}


static Obj ref_add_tag(Obj r, Tag t) {
  assert_ref_is_valid(r);
  assert(!(t & ot_val_mask)); // t must be a ref tag.
  return (Obj){ .u = r.u | t };
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


static Bool ref_is_func_host(Obj f) {
  return ref_struct_tag(f) == st_Func_host;
}


static Int ref_len(Obj r) {
  assert_ref_is_valid(r);
  assert(ref_struct_tag(r) == st_Data || ref_struct_tag(r) == st_Vec);
  assert(r.rcl->len > 0);
  return r.rcl->len;
}


static Ptr ref_body(Obj r) {
  assert_ref_is_valid(r);
  assert(ref_struct_tag(r) != st_Data && ref_struct_tag(r) != st_Vec);
  return r.rc + 1; // address past rc.
}


static Obj* ref_vec_els(Obj v) {
  assert(ref_is_vec(v));
  return cast(Obj*, v.rcl + 1); // address past rcl.
}


static B ref_data_ptr(Obj d) {
  assert(ref_is_data(d));
  return (B){.m = cast(BM, d.rcl + 1)}; // address past rcl.
}


static Obj ref_alloc(Struct_tag st, Int width) {
  // return Obj must be tagged ot_strong or else dealloced directly.
  assert(width > 0);
  Obj r = (Obj){.p=malloc(cast(Uns, width))};
  assert(!obj_tag(r)); // check that malloc is really aligned to the width of the tag.
  r.rc->st = st;
  r.rc->wc = 0;
  r.rc->mt = 0;
  r.rc->sc = 1;
  rc_errLV("alloc     ", r.rc);
#if OPT_ALLOC_COUNT
  total_allocs_ref++;
#endif
  return r;
}


static void ref_dealloc(Obj r) {
  assert_ref_is_valid(r);
  rc_errLV("dealloc   ", r.rc);
  check(r.rc->wc == 0, "attempt to deallocate object with non-zero weak count: %p", r.p);
  Struct_tag st = ref_struct_tag(r);
  if (st == st_Vec) {
    Obj* els = ref_vec_els(r);
    for_in(i, ref_len(r)) {
      obj_release(els[i]); // TODO: make this tail recursive for deallocating long chains?
    }
  }
#if OPT_DEALLOC_MARK
  r.rc->st = st_DEALLOC;
#endif
  free(r.p);
#if OPT_ALLOC_COUNT
  total_deallocs_ref++;
#endif
}


static void ref_release_strong(Obj r) {
  assert_ref_is_valid(r);
  if (r.rc->sc == 1) {
    ref_dealloc(r);
    return;
  }
  rc_errLV("rel strong", r.rc);
  if (r.rc->sc < pinned_sc) { // can only decrement if the count is not pinned.
    r.rc->sc--;
  }
}

              
static void ref_release_weak(Obj r) {
  assert_ref_is_valid(r);
  rc_errLV("rel weak ", r.rc);
  check_obj(r.rc->wc > 0, "over-released weak ref", r);
  // can only decrement if the count is not pinned.
  if (r.rc->wc < pinned_wc) r.rc->wc--;
}

