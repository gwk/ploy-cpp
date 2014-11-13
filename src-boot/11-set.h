// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// hash set data structure.

#include "10-list.h"


static const Int min_table_len_buckets = 16;

typedef List Hash_bucket;
DEF_SIZE(Hash_bucket);


class Set {
  Int _len;
  Int _len_buckets;
  Hash_bucket* _buckets;

public:
  Set(): _len(0), _len_buckets(0), _buckets(null) {}

  Set(Int l, Int lb, Hash_bucket* b): _len(l), _len_buckets(lb), _buckets(b) {}

  Bool vld() {
    if (_buckets) {
      return _len >= 0 && _len < _len_buckets;
    } else {
      return !_len && !_len_buckets;
    }
  }

  void dealloc(Bool dbg_cleared) {
    assert(vld());
    Int len_act = 0;
    for_in(i, _len_buckets) {
      len_act += _buckets[i].len();
      _buckets[i].dealloc(dbg_cleared);
    }
    assert(len_act == _len);
    raw_dealloc(_buckets, ci_Set);
  }

  Hash_bucket* bucket(Obj o) {
    assert(vld());
    assert(_len > 0);
    Int h = o.id_hash();
    Int i = h % _len_buckets;
    return _buckets + i;
  }

  Bool contains(Obj o) {
    assert(vld());
    if (!_len) return false;
    Hash_bucket* b = bucket(o);
    return b->contains(o);
  }

  void insert(Obj o) {
    assert(vld());
    assert(!contains(o)); // TODO: support duplicate insert.
    if (_len == 0) {
      _len_buckets = min_table_len_buckets;
      Int size = _len_buckets * size_Hash_bucket;
      _buckets = static_cast<Hash_bucket*>(raw_alloc(size, ci_Set));
      memset(_buckets, 0, Uns(size));
    } else if (_len + 1 == _len_buckets) { // load factor == 1.0.
      // TODO: assess resize criteria.
      Int s1_len_buckets = _len_buckets * 2;
      Int size = s1_len_buckets * size_Hash_bucket;
      Set s1 = Set(_len, s1_len_buckets, static_cast<Hash_bucket*>(raw_alloc(size, ci_Set)));
      memset(s1._buckets, 0, Uns(size));
      // copy existing elements.
      for_in(i, _len_buckets) {
        Hash_bucket src = _buckets[i];
        for_in(j, src.len()) {
          Obj el = src.el_move(j);
          Hash_bucket* dst = s1.bucket(el);
          dst->append(el);
        }
      }
      // replace set.
      dealloc(true);
      *this = s1;
    } else {
      assert(_len < _len_buckets);
    }
    _len++;
    Hash_bucket* b = bucket(o);
    b->append(o);
  }

  Obj remove(Obj o) {
    assert(vld());
    Hash_bucket* b = bucket(o);
    for_in(i, b->len()) {
      if (b->el(i) == o) {
        assert(_len > 0);
        _len--;
        return b->drop(i);
      }
    }
    assert(0);
    return obj0;
  }

};
DEF_SIZE(Set);
