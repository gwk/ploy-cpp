// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash set data structure.

#include "11-array.h"


static const Int min_set_len_buckets = 16;

typedef Array Hash_bucket;
DEF_SIZE(Hash_bucket);
static const Hash_bucket hash_bucket0 = array0;


typedef struct {
  Int len;
  Int len_buckets;
  Hash_bucket* buckets;
} Set;
DEF_SIZE(Set);

static const Set set0 = {.len=0, .len_buckets=0, .buckets=NULL};


static void assert_set_is_valid(Set* s) {
  assert(s);
  if (s->buckets) {
    assert(s->len >= 0 && s->len < s->len_buckets);
  } else {
    assert(s->len == 0 && s->len_buckets == 0);
  }
}


static void set_dealloc(Set* s) {
  assert_set_is_valid(s);
  for_in(i, s->len_buckets) {
    mem_dealloc(s->buckets[i].mem);
  }
  raw_dealloc(s->buckets, ci_Set);
}


static Hash_bucket* set_bucket(Set* s, Obj r) {
  assert_set_is_valid(s);
  assert(s->len > 0);
  Uns h = rc_hash(r);
  Uns i = h % cast(Uns, s->len_buckets);
  return s->buckets + i;
}


static Bool set_contains(Set* s, Obj r) {
  assert_set_is_valid(s);
  if (!s->len) return false;
  Hash_bucket* b = set_bucket(s, r);
  return array_contains(b, r);
}


static void set_insert(Set* s, Obj r) {
  assert_set_is_valid(s);
  if (s->len == 0) {
    s->len = 1;
    s->len_buckets = min_set_len_buckets;
    Int size = s->len_buckets * size_Hash_bucket;
    s->buckets = raw_alloc(size, ci_Set);
    memset(s->buckets, 0, size);
  } else if (s->len + 1 == s->len_buckets) { // load factor == 1.0.
    // TODO: assess resize criteria.
    Int len_buckets = s->len_buckets * 2;
    Int size = len_buckets * size_Hash_bucket;
    Set t = {
      .len = s->len + 1,
      .len_buckets = len_buckets,
      .buckets = raw_alloc(size, ci_Set),
    };
    memset(t.buckets, 0, size);
    // copy existing elements.
    for_in(i, s->len_buckets) {
      Hash_bucket src = s->buckets[i];
      for_in(j, src.mem.len) {
        Obj el = src.mem.els[j];
        Hash_bucket* dst = set_bucket(&t, el);
        array_append(dst, el);
      }
    }
    // replace set.
    assert(!set_contains(s, r));
    assert(!set_contains(&t, r));
    set_dealloc(s);
    *s = t;
  } else {
    assert(s->len < s->len_buckets);
  }
  Hash_bucket* b = set_bucket(s, r);
  array_append(b, r);
}


static void set_remove(Set* s, Obj r) {
  Hash_bucket* b = set_bucket(s, r);
  assert_array_is_valid(b);
  for_in(i, b->mem.len) {
    if (b->mem.els[i].r == r.r) {
      // replace r with the last element. no-op if len == 1.
      b->mem.els[i] = b->mem.els[b->mem.len - 1];
      b->mem.len--;
      return;
    }
  }
  assert(0);
}
