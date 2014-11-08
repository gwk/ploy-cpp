// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-bool.h"


struct Data {
  Ref_head head;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Data);


// zero length data word.
const Obj blank = (Obj){.u = ot_sym | data_word_bit};


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


static Bool data_ref_iso(Obj a, Obj b) {
  Int len = data_ref_len(a);
  return len == data_ref_len(b) && !memcmp(data_ref_ptr(a), data_ref_ptr(b), cast(Uns, len));
}


static Obj data_new_empty(Int len) {
  Obj d = ref_new(size_Data + len, rc_ret(t_Data));
  d.d->len = len;
  return d; // borrowed.
}


static Obj data_new_from_str(Str s) {
  if (!s.len) return rc_ret_val(blank);
  Obj d = data_new_empty(s.len);
  memcpy(data_ptr(d), s.chars, cast(Uns, s.len));
  return d;
}


static Obj data_new_from_chars(Chars_const c) {
  Uns len = strnlen(c, max_Int);
  check(len <= max_Int, "data_new_from_chars: string exceeded max length");
  Obj d = data_new_empty(cast(Int, len));
  memcpy(data_ptr(d), c, len);
  return d;
}


static Obj data_new_from_path(Chars_const path) {
  CFile f = fopen(path, "r");
  check(f, "could not open file: %s", path);
  fseek(f, 0, SEEK_END);
  Int len = ftell(f);
  if (!len) return rc_ret_val(blank);
  Obj d = data_new_empty(len);
  fseek(f, 0, SEEK_SET);
  Uns items_read = fread(data_ptr(d), size_Char, cast(Uns, len), f);
  check(cast(Int, items_read) == len,
        "read failed; expected len: %i; actual: %u; path: %s", len, items_read, path);
  fclose(f);
  return d;
}
