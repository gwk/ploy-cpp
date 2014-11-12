// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "16-bool.h"


struct Data {
  Head head;
  Int len;
} ALIGNED_TO_WORD;
DEF_SIZE(Data);


// zero length data word.
const Obj blank = Obj(Uns(ot_sym | data_word_bit));


static Int data_ref_len(Obj d) {
  assert(d.ref_is_data());
  assert(d.d->len > 0);
  return d.d->len;
}


static Int data_len(Obj d) {
  if (d == blank) return 0; // TODO: support all data-word values.
  return data_ref_len(d);
}


static Chars data_ref_chars(Obj d) {
  assert(d.ref_is_data());
  return reinterpret_cast<Chars>(d.d + 1); // address past data header.
}


static CharsM data_ref_charsM(Obj d) {
  assert(d.ref_is_data());
  return reinterpret_cast<CharsM>(d.d + 1); // address past data header.
}


static Chars data_chars(Obj d) {
  if (d == blank) return null; // TODO: support all data-word values.
  return data_ref_chars(d);
}


static Str data_str(Obj d) {
  return Str(data_len(d), data_chars(d));
}


static Bool data_ref_iso(Obj a, Obj b) {
  Int len = data_ref_len(a);
  return len == data_ref_len(b)
    && !memcmp(data_ref_chars(a), data_ref_chars(b), Uns(len));
}


static Obj data_new_raw(Int len) {
  counter_inc(ci_Data_ref_rc);
  Obj o = Obj(raw_alloc(size_Data + len, ci_Data_ref_alloc));
  o.h->type = t_Data.ret();
  o.h->rc = (1<<1) + 1;
  o.d->len = len;
  return o;
}


static Obj data_new_from_str(Str s) {
  if (!s.len) return blank.ret_val();
  Obj d = data_new_raw(s.len);
  memcpy(data_ref_charsM(d), s.chars, Uns(s.len));
  return d;
}


static Obj data_new_from_chars(Chars c) {
  Uns len = strnlen(c, max_Int);
  check(len <= max_Int, "data_new_from_chars: string exceeded max length");
  Obj d = data_new_raw(Int(len));
  memcpy(data_ref_charsM(d), c, len);
  return d;
}


static Obj data_new_from_path(Chars path) {
  CFile f = fopen(path, "r");
  check(f, "could not open file: %s", path);
  fseek(f, 0, SEEK_END);
  Int len = ftell(f);
  if (!len) return blank.ret_val();
  Obj d = data_new_raw(len);
  fseek(f, 0, SEEK_SET);
  Uns items_read = fread(data_ref_charsM(d), size_Char, Uns(len), f);
  check(Int(items_read) == len,
        "read failed; expected len: %i; actual: %u; path: %s", len, items_read, path);
  fclose(f);
  return d;
}
