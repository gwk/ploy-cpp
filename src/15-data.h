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


static Chars data_ptr(Obj d) {
  if (d.u == blank.u) return NULL; // TODO: support all data-word values.
  return ref_data_ptr(d);
}


static Str data_str(Obj d) {
  return str_mk(data_len(d), data_ptr(d));
}


static Obj data_empty(Int len) {
  Obj d = ref_alloc(st_Data, size_RCL + len);
  d.rcl->len = len;
  return d; // borrowed.
}


static Obj new_data_from_str(Str s) {
  if (!s.len) return obj_ret_val(blank);
  Obj d = data_empty(s.len);
  memcpy(data_ptr(d), s.chars, s.len);
  return d;
}


static Obj new_data_from_chars(Chars c) {
  return new_data_from_str(str_from_chars(c));
}


static Obj new_data_from_path(CharsC path) {
  CFile f = fopen(path, "r");
  check(f, "could not open file: %s", path);
  fseek(f, 0, SEEK_END);
  Int len = ftell(f);
  if (!len) return obj_ret_val(blank);
  Obj d = data_empty(len);
  fseek(f, 0, SEEK_SET);
  Uns items_read = fread(data_ptr(d), size_Char, cast(Uns, len), f);
  check(cast(Int, items_read) == len,
        "read failed; expected len: %ld; actual bytes: %lu", len, items_read);
  fclose(f);
  return d;
}

