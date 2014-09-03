// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// String struct type.
// By using a struct with an explicit length,
// we can create substrings without copying the original.

#include "05-chars.h"


typedef struct {
  Int len;
  Chars chars;
} Str;

#define str0 (Str){.len=0, .chars=NULL}

// for use with "%.*s" formatter.
#define FMT_STR(str) cast(I32, (str).len), (str).chars


static Str str_mk(Int len, Chars chars) {
  return (Str){.len=len, .chars=chars};
}


static Str str_from_chars(Chars p) {
  Uns len = strnlen(p, max_Int);
  check(len <= max_Int, "str_from_chars: string exceeded max length");
  return str_mk(cast(Int, len), p);
}


static void assert_str_is_valid(Str s) {
  assert((!s.chars && !s.len) || (s.chars && s.len > 0));
}


static void str_dealloc(Str s) {
  assert_str_is_valid(s);
  raw_dealloc(s.chars, ci_Str);
}


static Str str_alloc(Int len) {
  // add null terminator to the allocation for easier debugging; this does not affect len.
  Chars c = raw_alloc(len + 1, ci_Str);
  c[len] = 0;
  return str_mk(len, c);
}


static void str_realloc(Str* s, Int len) {
  assert_str_is_valid(*s);
  s->chars = raw_realloc(s->chars, len, ci_Str);
  s->len = len;
}


static void str_grow(Str* s) {
  assert(s->len > 0);
  str_realloc(s, s->len * 2);
}


static Int str_append(Str* s, Int i, Char c) {
  // returns updated index.
  // unlike Mem, the string capacity is stored in s.len, and the current position in i.
  assert(i <= s->len);
  if (i == s->len) str_grow(s);
  s->chars[i] = c;
  return i + 1;
}


static Bool str_eq(Str a, Str b) {
  assert_str_is_valid(a);
  assert_str_is_valid(b);
  return a.len == b.len && memcmp(a.chars, b.chars, cast(Uns, a.len)) == 0;
}


static Bool str_ends_with_char(Str s, Char c) {
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


static Int str_find_line_end(Str s, Int pos) {
  assert_str_is_valid(s);
  for_imn(i, pos, s.len) {
    if (s.chars[i] == '\n') {
      return i;
    }
  }
  return s.len;
}


static Str str_line_at_pos(Str s, Int pos) {
  Int from = str_find_line_start(s, pos);
  Int to = str_find_line_end(s, pos);
  //errFL("str_line_at_pos: len:%ld pos:%ld from:%ld to:%ld", s.len, pos, from, to);
  return str_slice(s, from, to);
}


static Chars str_src_loc_str(Str src, Str path, Int pos, Int len, Int line_num, Int col,
                             Chars msg) {
  // get source line.
  // caller is responsible for raw_dealloc of returned Chars.
  Str line = str_line_at_pos(src, pos);
  Chars_const nl = str_ends_with_char(line, '\n') ? "" : "\n";
  // create underline.
  Char under[len_buffer] = {}; // zeroes all elements.
  if (line.len < len_buffer) { // draw underline
    for_in(i, col) under[i] = ' ';
    if (len > 0) {
      for_in(i, len) under[col + i] = '~';
    } else {
      under[col] = '^';
    }
  }
  // create result.
  Chars s;
  Int s_len = asprintf(&s, "%.*s:%ld:%ld: %s\n%.*s%s%s\n",
                       FMT_STR(path), line_num + 1, col + 1, msg, FMT_STR(line), nl, under);
  counter_inc(ci_Chars); // matches asprinf.
  check(s_len > 0, "str_src_loc_str allocation failed: %s", msg);
  return s;
}
