// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference counting via hash table.

#include "07-obj.h"


static void assert_ref_is_valid(Obj o);

static Obj rc_ret(Obj o) {
  // increase the object's retain count by one.
  counter_inc(obj_counter_index(o));
  if (obj_tag(o)) return o;
  assert_ref_is_valid(o);
  rc_errMLV("ret strong", o.rc);
  if (o.rc->sc < pinned_sc - 1) {
    o.rc->sc++;
  } else if (o.rc->sc < pinned_sc) {
    o.rc->sc++;
    if (report_pinned_counts) {
      errFL("object strong count pinned: %p", o.p);
    }
  } // else previously pinned.
  return o;
}


static void ref_dealloc(Obj o);

static void rc_rel(Obj o) {
  // decrease the object's retain count by one, or deallocate it.
  counter_dec(obj_counter_index(o));
  if (obj_tag(o)) return;
  assert_ref_is_valid(o);
  if (o.rc->sc == 1) {
    ref_dealloc(o);
    return;
  }
  rc_errMLV("rel strong", o.rc);
  if (o.rc->sc < pinned_sc) { // can only decrement if the count is not pinned.
    o.rc->sc--;
  }
  rc_errMLV("rel s post", o.rc);
}


static Obj rc_ret_val(Obj o) {
  // ret counting for non-ref objects. a no-op for optimized builds.
  assert(obj_tag(o));
  counter_inc(obj_counter_index(o));
  return o;
}


static Obj rc_rel_val(Obj o) {
  // rel counting for non-ref objects. a no-op for optimized builds.
  assert(obj_tag(o));
  counter_dec(obj_counter_index(o));
  return o;
}


UNUSED_FN static Obj rc_ret_weak(Obj o) {
  if (obj_tag(o)) return o;
  assert_ref_is_valid(o);
  rc_errMLV("ret weak ", o.rc);
  if (o.rc->wc < pinned_wc - 1) {
    o.rc->wc++;
  } else if (o.rc->wc < pinned_wc) {
    o.rc->wc++;
    if (report_pinned_counts) {
      errFL("object weak count pinned: %p", o.p);
    }
  } // else previously pinned.
  return o;
}


UNUSED_FN static void rc_rel_weak(Obj o) {
  if (obj_tag(o)) return;
  assert_ref_is_valid(o);
  rc_errMLV("rel weak ", o.rc);
  check_obj(o.rc->wc > 0, "over-released weak ref", o);
  // can only decrement if the count is not pinned.
  if (o.rc->wc < pinned_wc) o.rc->wc--;
}
