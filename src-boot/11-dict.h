// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash map data structure.

#include "10-set.h"


struct Dict {
  Int len;
  Int len_buckets;
  Hash_bucket* buckets;

  Dict(): len(0), len_buckets(0), buckets(null) {}

  Dict(Int l, Int lb, Hash_bucket* b): len(l), len_buckets(lb), buckets(b) {}

  Bool vld() {
    if (buckets) {
      return len >= 0 && len < len_buckets;
    } else {
      return !len && !len_buckets;
    }
  }

  void rel() {
    assert(vld());
    for_in(i, len_buckets) {
      buckets[i].array.rel();
    }
  }

  void dealloc() {
    assert(vld());
    Int len_act = 0;
    for_in(i, len_buckets) {
      len_act += buckets[i].array.len / 2;
      buckets[i].array.dealloc();
    }
    assert(len_act == len);
    raw_dealloc(buckets, ci_Dict);
  }

  Hash_bucket* bucket(Obj k) {
    assert(vld());
    assert(len > 0);
    Int h = k.id_hash();
    Int i = h % len_buckets;
    return buckets + i;
  }

  Obj fetch(Obj k) {
    assert(vld());
    if (!len) return obj0;
    Hash_bucket* b = bucket(k);
    for_ins(i, b->array.len, 2) {
      if (b->array.els[i] == k) {
        return b->array.els[i + 1];
      }
    }
    return obj0;
  }

  Bool contains(Obj k) {
    return fetch(k).vld();
  }

  void insert(Obj k, Obj v) {
    assert(vld());
    Obj existing = fetch(k);
    if (v == existing) return;
    assert(!existing.vld());
    if (len == 0) {
      len_buckets = min_table_len_buckets;
      Int size = len_buckets * size_Hash_bucket;
      buckets = static_cast<Hash_bucket*>(raw_alloc(size, ci_Dict));
      memset(buckets, 0, Uns(size));
    } else if (len + 1 == len_buckets) { // load factor == 1.0.
      // TODO: assess resize criteria.
      Int d1_len_buckets = len_buckets * 2;
      Int size = d1_len_buckets * size_Hash_bucket;
      Dict d1 = Dict(len, d1_len_buckets, static_cast<Hash_bucket*>(raw_alloc(size, ci_Dict)));
      memset(d1.buckets, 0, Uns(size));
      // copy existing elements.
      for_in(i, len_buckets) {
        Hash_bucket src = buckets[i];
        for_ins(j, src.array.len, 2) {
          Obj ek = src.array.el_move(j);
          Obj ev = src.array.el_move(j + 1);
          Hash_bucket* dst = d1.bucket(ek);
          dst->append(ek);
          dst->append(ev);
        }
      }
      // replace dict.
      dealloc();
      *this = d1;
    } else {
      assert(len < len_buckets);
    }
    len++;
    Hash_bucket* b = bucket(k);
    b->append(k);
    b->append(v);
  }

  void remove(Obj k) {
    Hash_bucket* b = bucket(k);
    assert(vld());
    for_ins(i, b->array.len, 2) {
      if (b->array.els[i].r == k.r) {
        assert(len > 0);
        len--;
        // replace k,v pair with the last pair. no-op if len == 1.
        b->array.els[i] = b->array.els[b->array.len - 2]; // k.
        b->array.els[i + 1] = b->array.els[b->array.len - 1]; // v.
        b->array.len -= 2;
        return;
      }
    }
    assert(0); // TODO: support removing nonexistent key?
  }

};
DEF_SIZE(Dict);


