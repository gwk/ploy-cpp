// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash map data structure.

#include "13-set.h"


typedef struct {
  Int len;
  Int len_buckets;
  Hash_bucket* buckets;
} Dict;
DEF_SIZE(Dict);

static const Dict dict0 = {.len=0, .len_buckets=0, .buckets=NULL};


static void assert_dict_is_valid(Dict* d) {
  assert(d);
  if (d->buckets) {
    assert(d->len >= 0 && d->len < d->len_buckets);
  } else {
    assert(d->len == 0 && d->len_buckets == 0);
  }
}


DBG_FN static void dict_rel(Dict* d) {
  assert_dict_is_valid(d);
  for_in(i, d->len_buckets) {
    mem_rel(d->buckets[i].mem);
  }
}


static void dict_dealloc(Dict* d) {
  assert_dict_is_valid(d);
  Int len = 0;
  for_in(i, d->len_buckets) {
    len += d->buckets[i].mem.len / 2;
    mem_dealloc(d->buckets[i].mem);
  }
  assert(len == d->len);
  raw_dealloc(d->buckets, ci_Dict);
}


static Hash_bucket* dict_bucket(Dict* d, Obj k) {
  assert_dict_is_valid(d);
  assert(d->len > 0);
  Int h = obj_id_hash(k);
  Int i = h % d->len_buckets;
  return d->buckets + i;
}


static Obj dict_fetch(Dict* d, Obj k) {
  assert_dict_is_valid(d);
  if (!d->len) return obj0;
  Hash_bucket* b = dict_bucket(d, k);
  for_ins(i, b->mem.len, 2) {
    if (is(b->mem.els[i], k)) {
      return b->mem.els[i + 1];
    }
  }
  return obj0;
}


static Bool dict_contains(Dict* d, Obj k) {
  return !is(dict_fetch(d, k), obj0);
}


static void dict_insert(Dict* d, Obj k, Obj v) {
  assert_dict_is_valid(d);
  Obj existing = dict_fetch(d, k);
  if (is(v, existing)) return;
  assert(is(existing, obj0));
  if (d->len == 0) {
    d->len_buckets = min_table_len_buckets;
    Int size = d->len_buckets * size_Hash_bucket;
    d->buckets = cast(Hash_bucket*, raw_alloc(size, ci_Dict));
    memset(d->buckets, 0, cast(Uns, size));
  } else if (d->len + 1 == d->len_buckets) { // load factor == 1.0.
    // TODO: assess resize criteria.
    Int len_buckets = d->len_buckets * 2;
    Int size = len_buckets * size_Hash_bucket;
    Dict d1 = {
      .len = d->len,
      .len_buckets = len_buckets,
      .buckets = cast(Hash_bucket*, raw_alloc(size, ci_Dict)),
    };
    memset(d1.buckets, 0, cast(Uns, size));
    // copy existing elements.
    for_in(i, d->len_buckets) {
      Hash_bucket src = d->buckets[i];
      for_ins(j, src.mem.len, 2) {
        Obj ek = mem_el_move(src.mem, j);
        Obj ev = mem_el_move(src.mem, j + 1);
        Hash_bucket* dst = dict_bucket(&d1, ek);
        array_append(dst, ek);
        array_append(dst, ev);
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
  array_append(b, k);
  array_append(b, v);
}


UNUSED_FN static void dict_remove(Dict* d, Obj k) {
  Hash_bucket* b = dict_bucket(d, k);
  assert_array_is_valid(b);
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


