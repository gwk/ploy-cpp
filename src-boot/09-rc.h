// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference counting via hash table.
// unlike most hash tables, the hash is the full key;
// this is possible because it is just the object address with low zero bits shifted off.

#include "08-fmt.h"


static const Int load_factor_numer = 1;
static const Int load_factor_denom = 1;


struct RC_item {
  Obj r;
  union { // if the low bit is set, then it is a count; otherwise, it is a pointer to a count.
    Uns c; // count.
    RC_item* p;
  };
};
DEF_SIZE(RC_item);


struct RC_bucket {
  U32 len;
  U32 cap;
  RC_item* items;
};
DEF_SIZE(RC_bucket);


struct RC_table {
  Int len;
  Int len_buckets;
  RC_bucket* buckets;
};
DEF_SIZE(RC_table);


static const Int min_rc_len_bucket = size_min_alloc / size_Uns;
static const Int init_rc_len_buckets = (1<<12) / size_RC_bucket; // start with one page.


static RC_table rc_table = {};


#if OPTION_RC_TABLE_STATS

#define rc_hist_len_powers (width_word - 1)
#define rc_hist_len_collisions 256

static Int rc_hist_ref_count = 0; // total number of refs inserted into the table.
static Int rc_hist_ref_bits[width_word] = {}; // counts of each bit position in all refs.
static Int rc_hist_empties[rc_hist_len_powers] = {};
static Uns rc_hist_grows[rc_hist_len_powers] = {};
static Uns rc_hist_gets[rc_hist_len_collisions] = {};
static Uns rc_hist_moves[rc_hist_len_collisions] = {};


static void rc_hist_count_ref_bits(Obj r) {
  rc_hist_ref_count++;
  for_in(i, width_word) {
    Uns m = 1LU << i;
    rc_hist_ref_bits[i] += (r.u & m) && 1;
  }
}


static void rc_hist_count_empties() {
  Int i = int_pow2_fl(rc_table.len_buckets);
  if (i < 0) return;
  rc_hist_empties[i] = 0;
  for_in(j, rc_table.len_buckets) {
    if (rc_table.buckets[j].len == 0) rc_hist_empties[i]++;
  }
}


static void rc_hist_count_grows(U32 cap) {
  Int i = int_pow2_fl(cap);
  check(rc_hist_grows[i] < max_Uns, "rc_hist_grows[%ld] overflowed", i);
  rc_hist_grows[i]++;
}


static void rc_hist_count_gets(Int i) {
  rc_hist_gets[int_min(i, rc_hist_len_collisions)]++;
}


static void rc_hist_count_moves(Int i) {
  rc_hist_moves[i]++;
}


#else
#define rc_hist_count_ref_bits(o) ((void)0)
#define rc_hist_count_empties(i) ((void)0)
#define rc_hist_count_grows(i) ((void)0)
#define rc_hist_count_gets(i) ((void)0)
#define rc_hist_count_moves(i) ((void)0)
#endif


static Uns rc_hash(Obj r) {
  // pointer hash simply shifts off the bits that are guaranteed to be zero.
  assert(obj_is_ref(r));
  return r.u >> width_min_alloc;
}


static Bool rc_item_is_direct(RC_item* item) {
  assert(item);
  return item->c & 1; // check if the low flag bit is set.
}


static RC_bucket* rc_bucket_ptr(Obj r) {
  Uns h = rc_hash(r);
  Uns i = h % cast(Uns, rc_table.len_buckets);
  return rc_table.buckets + i;
}


static void rc_bucket_append(RC_bucket* b, Obj r, Uns c) {
  assert(b->len <= b->cap);
#if !OPT // check that r is not already in b.
  for_in(i, b->len) {
    RC_item* item = b->items + i;
    assert(item->r != r);
  }
#endif
  if (b->len == b->cap) { // grow.
    b->cap = (b->cap ? b->cap * 2 : min_rc_len_bucket);
    b->items = cast(RC_item*, raw_realloc(b->items, size_RC_item * b->cap, ci_RC_bucket));
    rc_hist_count_grows(b->cap);
  }
  check(b->len < max_U32, "RC_bucket overflowed");
  rc_hist_count_moves(b->len);
  b->items[b->len++] = (RC_item){.r=r, .c=c};
}


static void rc_bucket_remove(RC_bucket* b, Int i) {
  // first, replace this item with the last item. no-op if len == 1.
  b->items[i] = b->items[b->len - 1];
  b->len--; // then simply forget the original last item.
  assert(rc_table.len > 0);
  rc_table.len--;
}


static void rc_resize(Int len_buckets) {
  rc_hist_count_empties();
  Int old_len_buckets = rc_table.len_buckets;
  RC_bucket* old_buckets = rc_table.buckets;
  rc_table.len_buckets = len_buckets;
  Int size = size_RC_bucket * len_buckets;
  rc_table.buckets = cast(RC_bucket*, raw_alloc(size, ci_RC_table));
  memset(rc_table.buckets, 0, cast(Uns, size));
  // copy existing elements.
  for_in(i, old_len_buckets) {
    RC_bucket src = old_buckets[i];
    for_in(j, src.len) {
      RC_item item = src.items[j];
      rc_bucket_append(rc_bucket_ptr(item.r), item.r, item.c);
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


#if OPTION_ALLOC_COUNT
static void rc_cleanup() {
  for_in(i, rc_table.len_buckets) {
    raw_dealloc(rc_table.buckets[i].items, ci_RC_bucket);
  }
  raw_dealloc(rc_table.buckets, ci_RC_table);
}
#endif


static Obj rc_insert(Obj r) {
  // load factor == (numer / denom) == (num_items / num_buckets).
  if (rc_table.len * load_factor_denom >= rc_table.len_buckets * load_factor_numer) {
    rc_resize(rc_table.len_buckets * 2);
  }
  rc_hist_count_ref_bits(r);
  RC_bucket* b = rc_bucket_ptr(r);
  rc_bucket_append(b, r, (1<<1) + 1); // start with a direct count of 1 plus flag bit of 1.
  rc_table.len++;
  return r;
}


static RC_item* rc_resolve_item(RC_item* item) {
  // requires that the immediate delegate is direct.
  assert(item);
  if (rc_item_is_direct(item)) return item;
  RC_item* i = item->p;
  if (rc_item_is_direct(i)) return i;
  error("rc_resolve_item: doubly indirect item");
}


struct RC_BII {
  RC_bucket* bucket;
  RC_item* item;
  Int index;
};

static RC_BII rc_get_BII(Obj r) {
  // returns the resolved item but the original bucket and item index.
  assert(obj_is_ref(r));
  RC_bucket* b = rc_bucket_ptr(r);
  for_in(i, b->len) {
    RC_item* item = b->items + i;
    if (item->r == r) {
      rc_hist_count_gets(i);
      return (RC_BII){.bucket=b, .item=item, .index=i};
    }
  }
  // this is a legitimate case when we are releasing a cycle and have looped back around.
  return (RC_BII){};
}


static RC_item* rc_get_item(Obj r) {
  return rc_get_BII(r).item;
}


static void rc_delegate_item(RC_item* a, RC_item* b) {
  // make b indirect by delegating to a.
  assert(rc_item_is_direct(a));
  assert(rc_item_is_direct(b));
  Uns c = b->c - 1; // remove the tag bit, but do not shift.
  b->p = a;
  a->c += c; // a acquires the count from b.
  assert(rc_item_is_direct(a));
  assert(!rc_item_is_direct(b));
}


UNUSED_FN static void rc_delegate(Obj a, Obj b) {
  RC_item* ai = rc_resolve_item(rc_get_item(a));
  RC_item* bi = rc_get_item(b);
  check(rc_item_is_direct(bi), "rc_delegate: delegator has already delegated: %o", b);
  rc_delegate_item(ai, bi);
}


#if OPTION_ALLOC_COUNT
static void rc_remove(Obj r) {
  RC_BII bii = rc_get_BII(r);
  if (rc_item_is_direct(bii.item)) {
    Uns count = bii.item->c >> 1;
    if (count == 1) { // expected.
      rc_bucket_remove(bii.bucket, bii.index);
    } else { // leak.
      errFL("rc_remove: leak with rc: %u; " fmt_obj_dealloc_preserve, count, r);
    }
  } else {
    errFL("rc_remove: item has an indirect count: " fmt_obj_dealloc_preserve, r);
  }
}
#endif


static Uns rc_get(Obj o) {
  // get the object's retain count for debugging purposes.
  if (o.is_val()) return max_Uns;
  RC_item* item = rc_resolve_item(rc_get_item(o));
  return item->c >> 1; // shift off the flag bit.
}


static Obj rc_ret(Obj o) {
  // increase the object's retain count by one.
  assert(o.vld());
  counter_inc(obj_counter_index(o));
  if (o.is_val()) return o;
  RC_item* item = rc_get_item(o);
  check(item, "object was prematurely deallocated: %p", o);
  item = rc_resolve_item(item);
  assert(item->c < max_Uns);
  item->c += 2; // increment by two to leave the flag bit intact.
  return o;
}


static Obj ref_dealloc(Obj o);

static void rc_rel(Obj o) {
  // decrease the object's retain count by one, or deallocate it.
  assert(o.vld());
  do {
    if (o.is_val()) {
      counter_dec(obj_counter_index(o));
      return;
    }
    RC_BII bii = rc_get_BII(o);
    if (!bii.item) {
      // cycle deallocation is complete, and we have arrived at an already-deallocated member.
      return;     
    }
    counter_dec(obj_counter_index(o));
    RC_item* item = rc_resolve_item(bii.item);
    if (item->c == (1<<1) + 1) { // count == 1.
      rc_bucket_remove(bii.bucket, bii.index); // removes original item, not resolved.
      // the resolved item belongs to a different object in the cycle,
      // which will be presumably released and dealloced recursively by this call to dealloc.
      o = ref_dealloc(o); // returns tail object to be released.
    } else {
      item->c -= 2; // decrement by two to leave the flag bit intact.
      o = obj0;
    }
  } while (o.vld());
}


#if OPTION_ALLOC_COUNT
static void cmpd_dissolve_fields(Obj c);

static void rc_dissolve(Obj o) {
  if (obj_is_cmpd(o)) {
    cmpd_dissolve_fields(o);
  }
  rc_rel(o);
}
#endif


static Obj rc_ret_val(Obj o) {
  // ret counting for non-ref objects. a no-op for optimized builds.
  assert(o.is_val());
  counter_inc(obj_counter_index(o));
  return o;
}


static Obj rc_rel_val(Obj o) {
  // rel counting for non-ref objects. a no-op for optimized builds.
  assert(o.is_val());
  counter_dec(obj_counter_index(o));
  return o;
}


UNUSED_FN static void rc_dump(Chars identifier) {
  errFL("**** RC DUMP %s ****", identifier);
  for_in(i, rc_table.len_buckets) {
    for_in(j, rc_table.buckets[i].len) {
      RC_item* item = rc_table.buckets[i].items + j;
      Int id = rc_item_is_direct(item);
      errFL("  ref:%o direct:%i count:%i %s",
        item->r, id, (rc_resolve_item(item)->c >> 1), identifier);
    }
  }
  err_nl();
}


#if OPTION_RC_TABLE_STATS
static void rc_table_stats() {
  errFL("\n==== PLOY RC TABLE STATS ====");
  errFL("total refs: %ld; final buckets_len: %ld", rc_hist_ref_count, rc_table.len_buckets);

  errFL("\nref bits:");
  for_in(i, width_word) {
    Int c = rc_hist_ref_bits[i];
    errFL("%2ld: %6ld: %.2f", i , c, (Flt)c / rc_hist_ref_count);
  }
  
  errFL("\nempty buckets at resize:");
  for_in(i, rc_hist_len_powers) {
    Int len = 1L << i;
    Int e = rc_hist_empties[i];
    if (!e) continue;
    errFL("%2ld: %6ld / %6ld = %.2f", i, e, len, (Flt)e / len);
  }

  errFL("\nbucket grows:");
  Uns total_grows = 0;
  for_in(i, rc_hist_len_powers) {
    Uns g = rc_hist_grows[i];
    if (!g) continue;
    total_grows += g;
    errFL("%ld: %lu", i, g);
  }
  errFL("total grows: %lu", total_grows);

  errFL("\nitem gets (ret/rel):");
  Uns total_gets = 0;
  for_in(i, rc_hist_len_collisions) {
    Uns g = rc_hist_gets[i];
    if (!g) continue;
    total_gets += g;
    errFL("%ld: %lu", i, g);
  }
  errFL("total gets: %lu", total_gets);

  errFL("\nitem moves (insert/remove):");
  Uns total_moves = 0;
  for_in(i, rc_hist_len_collisions) {
    Uns g = rc_hist_moves[i];
    if (!g) continue;
    total_moves += g;
    errFL("%ld: %lu", i, g);
  }
  errFL("total moves: %lu", total_moves);  
}
#endif


