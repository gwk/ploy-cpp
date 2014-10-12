// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// String struct type.
// By using a struct with an explicit length,
// we can create substrings without copying the original.

#include "05-chars.h"


typedef struct {
  Int len;
  Chars_const chars;
} Str;

#define str0 (Str){.len=0, .chars=NULL}

// for use with "%.*s" formatter.
#define FMT_STR(str) cast(I32, (str).len), (str).chars


static Str str_mk(Int len, Chars_const chars) {
  return (Str){.len=len, .chars=chars};
}


static Str str_from_chars(Chars_const c) {
  Uns len = strnlen(c, max_Int);
  check(len <= max_Int, "str_from_chars: string exceeded max length");
  return str_mk(cast(Int, len), c);
}


static void assert_str_is_valid(Str s) {
  assert((!s.chars && !s.len) || (s.chars && s.len > 0));
}


static Bool str_eq(Str a, Str b) {
  assert_str_is_valid(a);
  assert_str_is_valid(b);
  return a.len == b.len && memcmp(a.chars, b.chars, cast(Uns, a.len)) == 0;
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
  return str_mk(to - from, s.chars + from);
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


static Chars str_src_loc_str(Str path, Str src, Int pos, Int len, Int line_num, Int col,
  Chars msg) {
  // get source line.
  // caller is responsible for raw_dealloc of returned Chars.
  Str line = str_line_at_pos_exc(src, pos);
  Chars_const opt_sp = (*msg ? " " : ""); // no space for empty message.
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
  Chars s;
  Int s_len = asprintf(&s, "%.*s:%ld:%ld:%s%s\n    %.*s\n    %s\n",
    FMT_STR(path), line_num + 1, col + 1, opt_sp, msg, FMT_STR(line), under);
  counter_inc(ci_Chars); // matches asprinf.
  check(s_len > 0, "str_src_loc_str allocation failed: %s", msg);
  return s;
}
