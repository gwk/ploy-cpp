// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "17-bool.h"


struct _Data {
  Obj type;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Data);


// zero length data word.
static const Obj blank = (Obj){.u = ot_sym | data_word_bit };


static Int data_ref_len(Obj d) {
  assert(ref_is_data(d));
  assert(d.d->len > 0);
  return d.d->len;
}


static Int data_len(Obj d) {
  if (is(d, blank)) return 0; // TODO: support all data-word values.
  return data_ref_len(d);
}


static Chars data_ref_ptr(Obj d) {
  assert(ref_is_data(d));
  return cast(Chars, d.d + 1); // address past data header.
}


static Chars data_ptr(Obj d) {
  if (is(d, blank)) return NULL; // TODO: support all data-word values.
  return data_ref_ptr(d);
}


static Str data_str(Obj d) {
  return str_mk(data_len(d), data_ptr(d));
}


static Obj data_empty(Int len) {
  Obj d = ref_alloc(size_Data + len);
  d.d->type = rc_ret(t_Data);
  d.d->len = len;
  return d; // borrowed.
}


static Obj data_new_from_str(Str s) {
  if (!s.len) return rc_ret_val(blank);
  Obj d = data_empty(s.len);
  memcpy(data_ptr(d), s.chars, s.len);
  return d;
}


static Obj data_new_from_chars(Chars c) {
  return data_new_from_str(str_from_chars(c));
}


static Obj data_new_from_path(Chars_const path) {
  CFile f = fopen(path, "r");
  check(f, "could not open file: %s", path);
  fseek(f, 0, SEEK_END);
  Int len = ftell(f);
  if (!len) return rc_ret_val(blank);
  Obj d = data_empty(len);
  fseek(f, 0, SEEK_SET);
  Uns items_read = fread(data_ptr(d), size_Char, cast(Uns, len), f);
  check(cast(Int, items_read) == len,
        "read failed; expected len: %ld; actual bytes: %lu", len, items_read);
  fclose(f);
  return d;
}
