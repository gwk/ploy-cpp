// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash map data structure.

#include "13-set.h"


struct Dict {
  Int len;
  Int len_buckets;
  Hash_bucket* buckets;
  Dict(Int l, Int lb, Hash_bucket* b): len(l), len_buckets(lb), buckets(b) {}

  Bool vld() {
    if (buckets) {
      return len >= 0 && len < len_buckets;
    } else {
      return !len && !len_buckets;
    }
  }
};
DEF_SIZE(Dict);

#define dict0 Dict(0, 0, null)


DBG_FN static void dict_rel(Dict* d) {
  assert(d->vld());
  for_in(i, d->len_buckets) {
    d->buckets[i].mem.rel();
  }
}


static void dict_dealloc(Dict* d) {
  assert(d->vld());
  Int len = 0;
  for_in(i, d->len_buckets) {
    len += d->buckets[i].mem.len / 2;
    d->buckets[i].mem.dealloc();
  }
  assert(len == d->len);
  raw_dealloc(d->buckets, ci_Dict);
}


static Hash_bucket* dict_bucket(Dict* d, Obj k) {
  assert(d->vld());
  assert(d->len > 0);
  Int h = k.id_hash();
  Int i = h % d->len_buckets;
  return d->buckets + i;
}


static Obj dict_fetch(Dict* d, Obj k) {
  assert(d->vld());
  if (!d->len) return obj0;
  Hash_bucket* b = dict_bucket(d, k);
  for_ins(i, b->mem.len, 2) {
    if (b->mem.els[i] == k) {
      return b->mem.els[i + 1];
    }
  }
  return obj0;
}


static Bool dict_contains(Dict* d, Obj k) {
  return dict_fetch(d, k).vld();
}


static void dict_insert(Dict* d, Obj k, Obj v) {
  assert(d->vld());
  Obj existing = dict_fetch(d, k);
  if (v == existing) return;
  assert(!existing.vld());
  if (d->len == 0) {
    d->len_buckets = min_table_len_buckets;
    Int size = d->len_buckets * size_Hash_bucket;
    d->buckets = cast(Hash_bucket*, raw_alloc(size, ci_Dict));
    memset(d->buckets, 0, cast(Uns, size));
  } else if (d->len + 1 == d->len_buckets) { // load factor == 1.0.
    // TODO: assess resize criteria.
    Int len_buckets = d->len_buckets * 2;
    Int size = len_buckets * size_Hash_bucket;
    Dict d1 = Dict(d->len, len_buckets, cast(Hash_bucket*, raw_alloc(size, ci_Dict)));
    memset(d1.buckets, 0, cast(Uns, size));
    // copy existing elements.
    for_in(i, d->len_buckets) {
      Hash_bucket src = d->buckets[i];
      for_ins(j, src.mem.len, 2) {
        Obj ek = src.mem.el_move(j);
        Obj ev = src.mem.el_move(j + 1);
        Hash_bucket* dst = dict_bucket(&d1, ek);
        dst->append(ek);
        dst->append(ev);
      }
    }
    // replace dict.
    dict_dealloc(d);
    *d = d1;
  } else {
    assert(d->len < d->len_buckets);
  }
  d->len++;
  Hash_bucket* b = dict_bucket(d, k);
  b->append(k);
  b->append(v);
}


UNUSED_FN static void dict_remove(Dict* d, Obj k) {
  Hash_bucket* b = dict_bucket(d, k);
  assert(b->vld());
  for_ins(i, b->mem.len, 2) {
    if (b->mem.els[i].r == k.r) {
      assert(d->len > 0);
      d->len--;
      // replace k,v pair with the last pair. no-op if len == 1.
      b->mem.els[i] = b->mem.els[b->mem.len - 2]; // k.
      b->mem.els[i + 1] = b->mem.els[b->mem.len - 1]; // v.
      b->mem.len -= 2;
      return;
    }
  }
  assert(0); // TODO: support removing nonexistent key?
}


