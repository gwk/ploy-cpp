// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference counting via hash table.
// unlike most hash tables, the hash is the full key;
// this is possible because it is just the object address with low zero bits shifted off.

#include "07-obj.h"


typedef struct {
  Uns h; // hash/key.
  Uns c; // count.
} RC_item;
DEF_SIZE(RC_item);


typedef struct {
  U32 len;
  U32 cap;
  RC_item* items;
} RC_bucket;
DEF_SIZE(RC_bucket);


typedef struct {
  Int len;
  Int len_buckets;
  RC_bucket* buckets;
} RC_table;
DEF_SIZE(RC_table);


static const Int min_rc_len_bucket = 1;
static const Int init_rc_len_buckets = (1<<12) / size_RC_bucket; // start with one page.


static RC_table rc_table = {};


static RC_bucket* rc_bucket_ptr(Uns h) {
  Uns i = h % cast(Uns, rc_table.len_buckets);
  return rc_table.buckets + i;
}


static void rc_bucket_append(RC_bucket* b, Uns h, Uns c) {
  if (b->len == b->cap) { // grow.
    b->cap = (b->cap ? b->cap * 2 : min_rc_len_bucket);
    b->items = raw_realloc(b->items, size_RC_item * b->cap, ci_RC_bucket);
  }
  b->items[b->len++] = (RC_item){.h=h, .c=c};
}


static void rc_resize(Int len_buckets) {
  Int old_len_buckets = rc_table.len_buckets;
  RC_bucket* old_buckets = rc_table.buckets;
  rc_table.len_buckets = len_buckets;
  Int size = size_RC_bucket * len_buckets;
  rc_table.buckets = raw_alloc(size, ci_RC_table);
  memset(rc_table.buckets, 0, size);
  // copy existing elements.
  for_in(i, old_len_buckets) {
    RC_bucket src = old_buckets[i];
    for_in(j, src.len) {
      RC_item item = src.items[j];
      rc_bucket_append(rc_bucket_ptr(item.h), item.h, item.c);
    }
    raw_dealloc(src.items, ci_RC_bucket);
  }
  raw_dealloc(old_buckets, ci_RC_table);
}


static void rc_init() {
  // initialize to a nonzero size so that we never have to check for the zero case.
  assert(init_rc_len_buckets > 0);
  rc_resize(init_rc_len_buckets);
}


#if OPT_ALLOC_COUNT
static void rc_cleanup() {
  for_in(i, rc_table.len_buckets) {
    raw_dealloc(rc_table.buckets[i].items, ci_RC_bucket);
  }
  raw_dealloc(rc_table.buckets, ci_RC_table);
}
#endif

static void rc_insert(Uns h) {
  if (rc_table.len == rc_table.len_buckets) { // load factor of 1; grow.
    rc_resize(rc_table.len_buckets * 2);
  }
  RC_bucket* b = rc_bucket_ptr(h);
  rc_bucket_append(b, h, 1);
}


static void assert_ref_is_valid(Obj o);

static Uns ref_hash(Obj r);

static Obj rc_ret(Obj o) {
  // increase the object's retain count by one.
  counter_inc(obj_counter_index(o));
  if (obj_tag(o)) return o;
  assert_ref_is_valid(o);
  Uns h = ref_hash(o);
  RC_bucket* b = rc_bucket_ptr(h);
  for_in(i, b->len) {
    RC_item* item = b->items + i;
    if (item->h == h) {
      assert(item->c > 0 && item->c < max_Uns);
      item->c++;
      return o;
    }
  }
  assert(0); // could not find object.
  return o;
}


static void ref_dealloc(Obj o);

static void rc_rel(Obj o) {
  // decrease the object's retain count by one, or deallocate it.
  counter_dec(obj_counter_index(o));
  if (obj_tag(o)) return;
  assert_ref_is_valid(o);
  Uns h = ref_hash(o);
  RC_bucket* b = rc_bucket_ptr(h);
  for_in(i, b->len) {
    RC_item* item = b->items + i;
    if (item->h == h) {
      assert(item->c > 0);
      if (item->c == 1) {
        // replace this item with the last item. no-op if len == 1.
        b->items[i] = b->items[b->len - 1];
        b->len--; // then simply forget the original last item.
        ref_dealloc(o);
      } else {
        item->c--;
      }
      return;
    }
  }
  assert(0); // could not find object.
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

