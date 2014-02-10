// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash set data structure.

#include "09-array.h"


static const Int min_set_len_buckets = 16;


typedef struct {
  Int len;
  Int cap;
  Obj* els; // TODO: union with Obj el to optimize the len == 1 case?
} Hash_bucket;
DEF_SIZE(Hash_bucket);

static const Hash_bucket hash_bucket0 = {.len=0, .cap=0, .els=NULL};


typedef struct {
  Int len;
  Int len_buckets;
  Hash_bucket* buckets;
} Set;
DEF_SIZE(Set);

static const Set set0 = {.len=0, .len_buckets=0, .buckets=NULL};


static void assert_hash_bucket_is_valid(Hash_bucket* b) {
  assert(b);
  if (b->els) {
    assert(b->len >= 0 && b->len <= b->cap);
  }
  else {
    assert(b->len == 0 && b->cap == 0);
  }
}


static void hash_bucket_dealloc(Hash_bucket* b) {
  assert_hash_bucket_is_valid(b);
  raw_dealloc(b->els, ci_Hash_bucket);
}


static Bool hash_bucket_contains(Hash_bucket* b, Obj r) {
  assert_hash_bucket_is_valid(b);
  for_in(i, b->len) {
    if (b->els[i].p == r.p) {
      return true;
    }
  }
  return false;
}


static void hash_bucket_append(Hash_bucket* b, Obj r) {
  assert(!hash_bucket_contains(b, r));
  if (b->len == b->cap) { // resize
    if (b->cap) {
      b->cap *= 2;
    }
    else {
      b->cap = 2;
    }
    b->els = raw_realloc(b->els, b->cap * size_Obj, ci_Hash_bucket);
#if OPT_ALLOC_SCRIBBLE
    memset(b->els + b->len, 0xAA, (b->cap - b->len) * size_Obj); // same value as OSX MallocPreScribble.
#endif
  }
  b->els[b->len++] = r;
}


static void hash_bucket_remove(Hash_bucket* b, Obj r) {
  assert_hash_bucket_is_valid(b);
  for_in(i, b->len) {
    if (b->els[i].p == r.p) {
      b->els[i] = b->els[b->len - 1]; // replace r with the last element. no-op if len == 1.
      b->len--;
      return;
    }
  }
  assert(0);
}


static void assert_set_is_valid(Set* s) {
  assert(s);
  if (s->buckets) {
    assert(s->len >= 0 && s->len < s->len_buckets);
  }
  else {
    assert(s->len == 0 && s->len_buckets == 0);
  }
}


static void set_dealloc(Set* s) {
  assert_set_is_valid(s);
  for_in(i, s->len_buckets) {
    hash_bucket_dealloc(s->buckets + i);
  }
  raw_dealloc(s->buckets, ci_Set);
}


static Hash_bucket* set_bucket(Set* s, Obj r) {
  assert_set_is_valid(s);
  assert(s->len > 0);
  Uns h = ref_hash(r);
  Uns i = h % cast(Uns, s->len_buckets);
  return s->buckets + i;
}


static Bool set_contains(Set* s, Obj r) {
  assert_set_is_valid(s);
  if (!s->len) return false;
  Hash_bucket* b = set_bucket(s, r);
  return hash_bucket_contains(b, r);
}


static void set_insert(Set* s, Obj r) {
  assert_set_is_valid(s);
  if (s->len == 0) {
    s->len = 1;
    s->len_buckets = min_set_len_buckets;
    Int size = s->len_buckets * size_Hash_bucket;
    s->buckets = raw_alloc(size, ci_Set);
    memset(s->buckets, 0, size);
  }
  else if (s->len + 1 == s->len_buckets) { // load factor == 1.0.
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
      for_in(j, src.len) {
        Obj el = src.els[j];
        Hash_bucket* dst = set_bucket(&t, el);
        hash_bucket_append(dst, el);
      }
    }
    // replace set.
    assert(!set_contains(s, r));
    assert(!set_contains(&t, r));
    set_dealloc(s);
    *s = t;
  }
  else {
    assert(s->len < s->len_buckets);
  }
  Hash_bucket* b = set_bucket(s, r);
  hash_bucket_append(b, r);
}


static void set_remove(Set* s, Obj r) {
  Hash_bucket* b = set_bucket(s, r);
  hash_bucket_remove(b, r);
}

