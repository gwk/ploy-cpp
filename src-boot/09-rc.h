// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference counting via hash table.
// unlike most hash tables, the hash is the full key;
// this is possible because it is just the object address with low zero bits shifted off.

#include "08-fmt.h"


static Obj rc_ret(Obj o) {
  // increase the object's retain count by one.
  assert(o.vld());
  counter_inc(o.counter_index());
  if (o.is_val()) return o;
  check(o.h->rc, "object was prematurely deallocated: %p", o.r);
  assert(o.h->rc & 1); // TODO: support indirect counts.
  assert(o.h->rc < max_Uns);
  o.h->rc += 2; // increment by two to leave the flag bit intact.
  return o;
}


static Obj ref_dealloc(Obj o);

static void rc_rel(Obj o) {
  // decrease the object's retain count by one, or deallocate it.
  assert(o.vld());
  do {
    if (o.is_val()) {
      counter_dec(o.counter_index());
      return;
    }
    if (!o.h->rc) {
      // cycle deallocation is complete, and we have arrived at an already-deallocated member.
      errFL("CYCLE: %p", o.r);
      assert(0); // TODO: support cycles.
      return;     
    }
    counter_dec(o.counter_index());
    assert(o.h->rc & 1); // TODO: support indirect counts.
    if (o.h->rc == (1<<1) + 1) { // count == 1.
      o = ref_dealloc(o); // returns tail object to be released.
    } else {
      o.h->rc -= 2; // decrement by two to leave the flag bit intact.
      break;
    }
  } while (o.vld());
}


#if OPTION_ALLOC_COUNT
static void cmpd_dissolve_fields(Obj c);

static void rc_dissolve(Obj o) {
  if (o.is_cmpd()) {
    cmpd_dissolve_fields(o);
  }
  rc_rel(o);
}
#endif


static Obj rc_ret_val(Obj o) {
  // ret counting for non-ref objects. a no-op for optimized builds.
  assert(o.is_val());
  counter_inc(o.counter_index());
  return o;
}


static Obj rc_rel_val(Obj o) {
  // rel counting for non-ref objects. a no-op for optimized builds.
  assert(o.is_val());
  counter_dec(o.counter_index());
  return o;
}

