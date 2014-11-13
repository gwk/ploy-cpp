// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash map data structure.

#include "11-set.h"


class Dict {
  Int _len;
  Int _len_buckets;
  Hash_bucket* _buckets;

public:

  Dict(): _len(0), _len_buckets(0), _buckets(null) {}

  Dict(Int l, Int lb, Hash_bucket* b): _len(l), _len_buckets(lb), _buckets(b) {}

  Bool vld() {
    if (_buckets) {
      return _len >= 0 && _len < _len_buckets;
    } else {
      return !_len && !_len_buckets;
    }
  }

  void rel_els() {
    assert(vld());
    for_in(i, _len_buckets) {
      _buckets[i].rel_els();
    }
  }

  void dealloc() {
    assert(vld());
    Int len_act = 0;
    for_in(i, _len_buckets) {
      len_act += _buckets[i].len();
      _buckets[i].dealloc();
    }
    assert(len_act == _len * 2);
    raw_dealloc(_buckets, ci_Dict);
  }

  Hash_bucket* bucket(Obj k) {
    assert(vld());
    assert(_len > 0);
    Int h = k.id_hash();
    Int i = h % _len_buckets;
    return _buckets + i;
  }

  Obj fetch(Obj k) {
    assert(vld());
    if (!_len) return obj0;
    Hash_bucket* b = bucket(k);
    for_ins(i, b->len(), 2) {
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
    if (_len == 0) {
      _len_buckets = min_table_len_buckets;
      Int size = _len_buckets * size_Hash_bucket;
      _buckets = static_cast<Hash_bucket*>(raw_alloc(size, ci_Dict));
      memset(_buckets, 0, Uns(size));
    } else if (_len + 1 == _len_buckets) { // load factor == 1.0.
      // TODO: assess resize criteria.
      Int d1_len_buckets = _len_buckets * 2;
      Int size = d1_len_buckets * size_Hash_bucket;
      Dict d1 = Dict(_len, d1_len_buckets, static_cast<Hash_bucket*>(raw_alloc(size, ci_Dict)));
      memset(d1._buckets, 0, Uns(size));
      // copy existing elements.
      for_in(i, _len_buckets) {
        Hash_bucket src = _buckets[i];
        for_ins(j, src.len(), 2) {
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
      assert(_len < _len_buckets);
    }
    _len++;
    Hash_bucket* b = bucket(k);
    b->append(k);
    b->append(v);
  }

  Obj remove(Obj k) {
    Hash_bucket* b = bucket(k);
    assert(vld());
    for_ins(ik, b->len(), 2) {
      if (b->el(ik).r == k.r) {
        assert(_len > 0);
        _len--;
        Obj o = b->drop(ik + 1); // val first, so that last val gets swapped in.
        b->drop(ik); // key second; last key is now the last el.
        return o;
      }
    }
    assert(0); // TODO: support removing nonexistent key?
    return obj0;
  }

};
DEF_SIZE(Dict);


