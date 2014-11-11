// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash set data structure.

#include "12-array.h"


static const Int min_table_len_buckets = 16;

typedef Array Hash_bucket;
DEF_SIZE(Hash_bucket);
static const Hash_bucket hash_bucket0 = array0;


struct Set {
  Int len;
  Int len_buckets;
  Hash_bucket* buckets;

  Set(Int l, Int lb, Hash_bucket* b): len(l), len_buckets(lb), buckets(b) {}

  Bool vld() {
    if (buckets) {
      return len >= 0 && len < len_buckets;
    } else {
      return !len && !len_buckets;
    }
  }
};
DEF_SIZE(Set);

#define set0 Set(0, 0, null)


static void set_dealloc(Set* s) {
  assert(s->vld());
  Int len = 0;
  for_in(i, s->len_buckets) {
    len += s->buckets[i].mem.len;
    s->buckets[i].mem.dealloc();
  }
  assert(len == s->len);
  raw_dealloc(s->buckets, ci_Set);
}


static Hash_bucket* set_bucket(Set* s, Obj o) {
  assert(s->vld());
  assert(s->len > 0);
  Int h = o.id_hash();
  Int i = h % s->len_buckets;
  return s->buckets + i;
}


static Bool set_contains(Set* s, Obj o) {
  assert(s->vld());
  if (!s->len) return false;
  Hash_bucket* b = set_bucket(s, o);
  return array_contains(b, o);
}


static void set_insert(Set* s, Obj o) {
  assert(s->vld());
  assert(!set_contains(s, o)); // TODO: support duplicate insert.
  if (s->len == 0) {
    s->len_buckets = min_table_len_buckets;
    Int size = s->len_buckets * size_Hash_bucket;
    s->buckets = cast(Hash_bucket*, raw_alloc(size, ci_Set));
    memset(s->buckets, 0, cast(Uns, size));
  } else if (s->len + 1 == s->len_buckets) { // load factor == 1.0.
    // TODO: assess resize criteria.
    Int len_buckets = s->len_buckets * 2;
    Int size = len_buckets * size_Hash_bucket;
    Set s1 = Set(s->len, len_buckets, cast(Hash_bucket*, raw_alloc(size, ci_Set)));
    memset(s1.buckets, 0, cast(Uns, size));
    // copy existing elements.
    for_in(i, s->len_buckets) {
      Hash_bucket src = s->buckets[i];
      for_in(j, src.mem.len) {
        Obj el = src.mem.el_move(j);
        Hash_bucket* dst = set_bucket(&s1, el);
        array_append(dst, el);
      }
    }
    // replace set.
    set_dealloc(s);
    *s = s1;
  } else {
    assert(s->len < s->len_buckets);
  }
  s->len++;
  Hash_bucket* b = set_bucket(s, o);
  array_append(b, o);
}


static void set_remove(Set* s, Obj o) {
  Hash_bucket* b = set_bucket(s, o);
  assert(b->vld());
  for_in(i, b->mem.len) {
    if (b->mem.els[i] == o) {
      assert(s->len > 0);
      s->len--;
      // replace o with the last element. no-op if len == 1.
      b->mem.els[i] = b->mem.els[b->mem.len - 1];
      b->mem.len--;
      return;
    }
  }
  assert(0);
}
