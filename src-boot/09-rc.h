// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// reference counting via hash table.
// unlike most hash tables, the hash is the full key;
// this is possible because it is just the object address with low zero bits shifted off.

#include "08-fmt.h"

static const Int load_factor_numer = 1;
static const Int load_factor_denom = 1;


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


static const Int min_rc_len_bucket = size_min_alloc / size_Uns;
static const Int init_rc_len_buckets = (1<<12) / size_RC_bucket; // start with one page.


static RC_table rc_table = {};


#if OPT_RC_TABLE_STATS

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


static RC_bucket* rc_bucket_ptr(Uns h) {
  Uns i = h % cast(Uns, rc_table.len_buckets);
  return rc_table.buckets + i;
}


static void rc_bucket_append(RC_bucket* b, Uns h, Uns c) {
  assert(b->len <= b->cap);
  if (b->len == b->cap) { // grow.
    b->cap = (b->cap ? b->cap * 2 : min_rc_len_bucket);
    b->items = raw_realloc(b->items, size_RC_item * b->cap, ci_RC_bucket);
    rc_hist_count_grows(b->cap);
  }
  check(b->len < max_U32, "RC_bucket overflowed");
  rc_hist_count_moves(b->len);
  b->items[b->len++] = (RC_item){.h=h, .c=c};
}


static void rc_resize(Int len_buckets) {
  rc_hist_count_empties();
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


static Uns rc_hash(Obj r) {
  // pointer hash simply shifts of the bits that are guaranteed to be zero.
  return r.u >> width_min_alloc;
}



static void rc_insert(Obj r) {
  // load factor == (numer / denom) == (num_items / num_buckets).
  if (rc_table.len * load_factor_denom >= rc_table.len_buckets * load_factor_numer) {
    rc_resize(rc_table.len_buckets * 2);
  }
  rc_hist_count_ref_bits(r);
  Uns h = rc_hash(r);
  RC_bucket* b = rc_bucket_ptr(h);
  rc_bucket_append(b, h, 1);
  rc_table.len++;
}


static void rc_remove(RC_bucket* b, Int i) {
  // replace this item with the last item. no-op if len == 1.
  b->items[i] = b->items[b->len - 1];
  b->len--; // then simply forget the original last item.
  assert(rc_table.len > 0);
  rc_table.len--;
}


static Obj rc_ret(Obj o) {
  // increase the object's retain count by one.
  counter_inc(obj_counter_index(o));
  if (obj_tag(o)) return o;
  Uns h = rc_hash(o);
  RC_bucket* b = rc_bucket_ptr(h);
  for_in(i, b->len) {
    RC_item* item = b->items + i;
    if (item->h == h) {
      assert(item->c > 0 && item->c < max_Uns);
      item->c++;
      rc_hist_count_gets(i);
      return o;
    }
  }
  assert(0); // could not find object.
  return o;
}


static Obj ref_dealloc(Obj o);

static void rc_rel(Obj o) {
  // decrease the object's retain count by one, or deallocate it.
  assert(!is(o, obj0));
  do {
    counter_dec(obj_counter_index(o));
    if (obj_tag(o)) return;
    Int round = 0;
    Bool found = false;
    Uns h = rc_hash(o);
    RC_bucket* b = rc_bucket_ptr(h);
    for_in(i, b->len) {
      RC_item* item = b->items + i;
      if (item->h == h) {
        assert(item->c > 0);
        found = true;
        rc_hist_count_gets(i);
        if (item->c == 1) {
          rc_remove(b, i);
          o = ref_dealloc(o); // returns tail object to be released.
        } else {
          item->c--;
          o = obj0;
        }
        break;
      }
    }
    assert(found);
    round++;
  } while (!is(o, obj0));
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


#if OPT_RC_TABLE_STATS
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


