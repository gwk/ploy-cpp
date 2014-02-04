// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "09-array.h"

// data structure for a simple append-only set of ref pointers.

static const Int min_set_len = 16;


typedef struct {
  Int len;
  Int cap;
  Obj* els; // TODO: union Obj el?
} Hash_bucket;

static const Int size_Hash_bucket = sizeof(Hash_bucket);
static const Hash_bucket hash_bucket0 = {.len=0, .cap=0, .els=NULL};


typedef struct {
  Int count;
  Int len;
  Hash_bucket* buckets;
} Set;

static const Set set0 = {.count=0, .len=0, .buckets=NULL};


static void hash_bucket_dealloc(Hash_bucket* b) {
  raw_dealloc(b->els, ci_Hash_bucket);
}


static bool hash_bucket_contains(Hash_bucket* b, Obj r) {
  assert(b);
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
      b->cap = size_min_malloc;
    }
    b->els = raw_realloc(b->els, b->cap * size_Obj, ci_Hash_bucket);
#if OPT_CLEAR_ELS
    memset(b->els + b->len, 0, (b->cap - b->len) * size_Obj);
#endif
  }
  b->els[b->len++] = r;
}


static void set_dealloc(Set* s) {
  assert(s);
  for_in(i, s->len) {
    hash_bucket_dealloc(s->buckets + i);
  }
  raw_dealloc(s->buckets, ci_Set);
    free(s->buckets);
}


static Hash_bucket* set_bucket(Set* s, Obj r) {
  assert(s);
  Uns h = ref_hash(r);
  Uns i = h % cast(Uns, s->len);
  return s->buckets + i;
}


static bool set_contains(Set* s, Obj r) {
  assert(s);
  if (!s->len) return false;
  Hash_bucket* b = set_bucket(s, r);
  return hash_bucket_contains(b, r);
}


static void set_insert(Set* s, Obj r) {
  assert(s);
  s->count++;
  if (!s->len) {
    s->len = min_set_len;
    s->buckets = malloc(cast(Uns, s->len * size_Hash_bucket));
  }
  else if (s->count >= s->len) { // load factor >= 1.0.
    // TODO: assess resize criteria.
    Int len = s->len * 2;
    Set t = {
      .count = s->count,
      .len = len,
      .buckets = malloc(cast(Uns, len * size_Hash_bucket))
    };
    // copy existing elements.
    for_in(i, s->len) {
      Hash_bucket b_old = s->buckets[i];
      for_in(j, b_old.len) {
        Obj el = b_old.els[j];
        Hash_bucket* b = set_bucket(&t, el);
        hash_bucket_append(b, el);
      }
    }
    // replace set.
    set_dealloc(s);
    *s = t;
  }
  Hash_bucket* b = set_bucket(s, r);
  hash_bucket_append(b, r);
}

