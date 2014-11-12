// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash set data structure.

#include "09-list.h"


static const Int min_table_len_buckets = 16;

typedef List Hash_bucket;
DEF_SIZE(Hash_bucket);


struct Set {
  Int len;
  Int len_buckets;
  Hash_bucket* buckets;

  Set(): len(0), len_buckets(0), buckets(null) {}

  Set(Int l, Int lb, Hash_bucket* b): len(l), len_buckets(lb), buckets(b) {}

  Bool vld() {
    if (buckets) {
      return len >= 0 && len < len_buckets;
    } else {
      return !len && !len_buckets;
    }
  }

  void dealloc(Bool assert_cleared) {
    assert(vld());
    Int len_act = 0;
    for_in(i, len_buckets) {
      len_act += buckets[i].array.len;
      if (assert_cleared) {
        buckets[i].array.dealloc();
      } else {
        buckets[i].array.dealloc_no_clear();
      }
    }
    assert(len_act == len);
    raw_dealloc(buckets, ci_Set);
  }

  Hash_bucket* bucket(Obj o) {
    assert(vld());
    assert(len > 0);
    Int h = o.id_hash();
    Int i = h % len_buckets;
    return buckets + i;
  }

  Bool contains(Obj o) {
    assert(vld());
    if (!len) return false;
    Hash_bucket* b = bucket(o);
    return b->contains(o);
  }

  void insert(Obj o) {
    assert(vld());
    assert(!contains(o)); // TODO: support duplicate insert.
    if (len == 0) {
      len_buckets = min_table_len_buckets;
      Int size = len_buckets * size_Hash_bucket;
      buckets = static_cast<Hash_bucket*>(raw_alloc(size, ci_Set));
      memset(buckets, 0, Uns(size));
    } else if (len + 1 == len_buckets) { // load factor == 1.0.
      // TODO: assess resize criteria.
      Int s1_len_buckets = len_buckets * 2;
      Int size = s1_len_buckets * size_Hash_bucket;
      Set s1 = Set(len, s1_len_buckets, static_cast<Hash_bucket*>(raw_alloc(size, ci_Set)));
      memset(s1.buckets, 0, Uns(size));
      // copy existing elements.
      for_in(i, len_buckets) {
        Hash_bucket src = buckets[i];
        for_in(j, src.array.len) {
          Obj el = src.array.el_move(j);
          Hash_bucket* dst = s1.bucket(el);
          dst->append(el);
        }
      }
      // replace set.
      dealloc(true);
      *this = s1;
    } else {
      assert(len < len_buckets);
    }
    len++;
    Hash_bucket* b = bucket(o);
    b->append(o);
  }

  void remove(Obj o) {
    assert(vld());
    Hash_bucket* b = bucket(o);
    for_in(i, b->array.len) {
      if (b->array.els[i] == o) {
        assert(len > 0);
        len--;
        // replace o with the last element. no-op if len == 1.
        b->array.els[i] = b->array.els[b->array.len - 1];
        b->array.len--;
        return;
      }
    }
    assert(0);
  }

};
DEF_SIZE(Set);
