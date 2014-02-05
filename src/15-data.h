// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "14-bool.h"


// zero length data word.
static const Obj blank = (Obj){.u = ot_data };


#define assert_is_data(d) \
assert(d.u == blank.u || obj_tag(d) == ot_data);


static Int data_ref_len(Obj d) {
  assert(ref_is_data(d));
  assert(d.rcl->len > 0);
  return d.rcl->len;
}


static Int data_len(Obj d) {
  if (d.u == blank.u) return 0; // TODO: support all data-word values.
  return data_ref_len(d);
}


static B data_ptr(Obj d) {
  if (d.u == blank.u) return (B){.c=NULL}; // TODO: supoprt all data-word values.
  return ref_data_ptr(d);
}


static SS data_SS(Obj d) {
  return ss_mk(data_len(d), data_ptr(d));
}


static Obj data_empty(Int len) {
  Obj d = ref_alloc(st_Data, size_RCL + len);
  d.rcl->len = len;
  return d; // borrowed.
}


static Obj new_data_from_SS(SS s) {
  if (!s.len) return obj_ret_val(blank);
  Obj d = data_empty(s.len);
  memcpy(data_ptr(d).m, s.b.c, s.len);
  return d;
}


static Obj new_data_from_BC(BC bc) {
  return new_data_from_SS(ss_from_BC(bc));
}


static Obj new_data_from_path(BC path) {
  File f = fopen(path, "r");
  check(f, "could not open file: %s", path);
  fseek(f, 0, SEEK_END);
  Int len = ftell(f);
  if (!len) return obj_ret_val(blank);
  Obj d = data_empty(len);
  fseek(f, 0, SEEK_SET);
  Uns items_read = fread(data_ptr(d).m, size_Char, cast(Uns, len), f);
  check(cast(Int, items_read) == len, "read failed; expected len: %ld; actual bytes: %lu", len, items_read);
  fclose(f);
  return d;
}
