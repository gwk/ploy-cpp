// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash map data structure.

#include "11-set.h"


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

  void rel_els() {
    assert(vld());
    for_in(i, len_buckets) {
      buckets[i].rel_els();
    }
  }

  void dealloc() {
    assert(vld());
    Int len_act = 0;
    for_in(i, len_buckets) {
      len_act += buckets[i].len / 2;
      buckets[i].dealloc();
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
    for_ins(i, b->len, 2) {
      if (b->el(i) == k) {
        return b->el(i + 1);
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
        for_ins(j, src.len, 2) {
          Obj ek = src.el_move(j);
          Obj ev = src.el_move(j + 1);
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
    for_ins(ik, b->len, 2) {
      if (b->el(ik).r == k.r) {
        assert(len > 0);
        len--;
        // replace k,v pair with the last pair. no-op if len == 1.
        Int iv = ik + 1;
        Int last_ik = b->len - 2;
        Int last_iv = last_ik + 1;
        b->el_move(ik);
        b->el_move(iv);
        if (ik < last_ik) { // move last pair into this position.
          b->put(ik, b->el_move(last_ik));
          b->put(iv, b->el_move(last_iv));
        }
        b->len -= 2;
        return;
      }
    }
    assert(0); // TODO: support removing nonexistent key?
  }

};
DEF_SIZE(Dict);


