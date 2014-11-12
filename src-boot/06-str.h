// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// String struct type.
// By using a struct with an explicit length,
// we can create substrings without copying the original.

#include "05-chars.h"


struct Str {
  Int len;
  Chars chars;
  Str(Int l, Chars c): len(l), chars(c) { assert(l > 0 || (!l && !c)); }
  
  explicit Str(Chars c) {
    Uns l = strnlen(c, max_Int);
    check(l <= max_Int, "Str: Chars overflowed max_Int; len: %u", l);
    chars = l ? c : null;
    len = Int(l);
  }
};

#define str0 Str(0, null)

// for use with "%.*s" formatter.
#define FMT_STR(str) I32((str).len), (str).chars


static void assert_str_is_valid(Str s) {
  assert((!s.chars && !s.len) || (s.chars && s.len > 0));
}


static Bool str_eq(Str a, Str b) {
  assert_str_is_valid(a);
  assert_str_is_valid(b);
  return a.len == b.len && memcmp(a.chars, b.chars, Uns(a.len)) == 0;
}


UNUSED_FN static Bool str_ends_with_char(Str s, Char c) {
  assert_str_is_valid(s);
  return (s.len > 0 && s.chars[s.len - 1] == c);
}


static Str str_slice(Str s, Int from, Int to) {
  assert_str_is_valid(s);
  assert(from >= 0);
  assert(to >= 0);
  if (from > s.len) from = s.len;
  if (from >= to) return str0;
  return Str(to - from, s.chars + from);
}


static Int str_find_line_start(Str s, Int pos) {
  assert_str_is_valid(s);
  for_in_rev(i, pos) {
    if (s.chars[i] == '\n') {
      return i + 1;
    }
  }
  return 0;
}


static Int str_find_line_end_exc(Str s, Int pos) {
  assert_str_is_valid(s);
  for_imn(i, pos, s.len) {
    if (s.chars[i] == '\n') {
      return i;
    }
  }
  return s.len;
}


static Str str_line_at_pos_exc(Str s, Int pos) {
  Int from = str_find_line_start(s, pos);
  Int to = str_find_line_end_exc(s, pos);
  //errFL("str_line_at_pos: len:%ld pos:%ld from:%ld to:%ld", s.len, pos, from, to);
  return str_slice(s, from, to);
}


static CharsM str_src_loc_str(Str path, Str src, Int pos, Int len, Int line_num, Int col,
  Chars msg) {
  // get source line.
  // caller is responsible for raw_dealloc of returned CharsM.
  Str line = str_line_at_pos_exc(src, pos);
  Chars opt_sp = (*msg ? " " : ""); // no space for empty message.
  // create underline.
  Char under[len_buffer] = {}; // zeroes all elements.
  if (line.len < len_buffer) { // draw underline
    for_in(i, col) under[i] = ' ';
    if (len > 0) {
      Int last = line.len - col;
      if (len <= last) {
        assert(col + len <= len_buffer);
        for_in(i, len) under[col + i] = '~';
      } else {
        assert(col + last < len_buffer);
        for_in(i, last) under[col + i] = '~';
        under[col + last] = '-';
      }
    } else {
      assert(col < len_buffer);
      under[col] = '^';
    }
  }
  // create result.
  CharsM s;
  Int s_len = asprintf(&s, "%.*s:%ld:%ld:%s%s\n    %.*s\n    %s\n",
    FMT_STR(path), line_num + 1, col + 1, opt_sp, msg, FMT_STR(line), under);
  counter_inc(ci_Chars); // matches asprinf.
  check(s_len > 0, "str_src_loc_str allocation failed: %s", msg);
  return s;
}
