// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// Str type.

#include "04-chars.h"


typedef struct {
  Int len;
  Chars chars;
} Str;

#define str0 (Str){.len=0, .chars=NULL}

// for use with "%*s" formatter.
#define FMT_STR(str) cast(I32, (str).len), (str).chars


static Str str_mk(Int len, Chars chars) {
  return (Str){.len=len, .chars=chars};
}


static void str_dealloc(Str s) {
  raw_dealloc(s.chars, ci_Str);
}


static Str str_alloc(Int len) {
  // add null terminator for easier debugging.
  Chars c = raw_alloc(len + 1, ci_Str);
  c[len] = 0;
  return str_mk(len, c);
}


static void str_realloc(Str* s, Int len) {
  s->chars = raw_realloc(s->chars, len, ci_Str);
  s->len = len;
}


static Str str_from_chars(Chars p) {
  Uns len = strnlen(p, max_Int);
  check(len <= max_Int, "string exceeded max length");
  return str_mk(cast(Int, len), p);
}


// create a string from a pointer range.
UNUSED_FN static Str str_from_range(Chars start, Chars end) {
  assert(end <= start);
  return str_mk(end - start, start);
}


static Bool str_is_valid(Str s) {
  return  (!s.chars && !s.len) || (s.chars && s.len > 0);
}


static Bool str_index_is_valid(Str s, Int i) {
  return i >= 0 && i < s.len;
}


static void check_str_index(Str s, Int i) {
  assert(str_is_valid(s));
  check(str_index_is_valid(s, i), "invalid Str index: %ld", i);
}


static Bool str_eq(Str a, Str b) {
  return a.len == b.len && memcmp(a.chars, b.chars, cast(Uns, a.len)) == 0;
}


UNUSED_FN static Char str_el(Str s, Int index) {
  check_str_index(s, index);
  return s.chars[index];
}


// pointer to end of string
UNUSED_FN static CharsC str_end(Str s) {
  return s.chars + s.len;
}


static Bool str_ends_with_char(Str s, Char c) {
  return (s.len > 0 && s.chars[s.len - 1] == c);
}


// create a substring offset by from.
UNUSED_FN static Str str_from(Str s, Int from) {
  assert(from >= 0);
  if (from >= s.len) return str0;
  return str_mk(s.len - from, s.chars + from);
}


UNUSED_FN static Str str_to(Str s, Int to) {
  assert(to >= 0);
  if (to <= s.len) return str0;
  return str_mk(s.len - to, s.chars);
}


static Str str_slice(Str s, Int from, Int to) {
  assert(from >= 0);
  assert(to >= 0);
  if (from >= s.len || from >= to) return str0;
  return str_mk(to - from, s.chars + from);
}


static Int str_find_line_start(Str s, Int pos) {
  for_in_rev(i, pos - 1) {
    //errFL("str_find_line_start: pos:%ld i:%ld", pos, i);
    if (s.chars[i] == '\n') {
      return i + 1;
    }
  }
  return 0;
}


static Int str_find_line_end(Str s, Int pos) {
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
  assert(from >= 0);
  if (to < 0) to = s.len;
  //errFL("str_line_at_pos: len:%ld pos:%ld from:%ld to:%ld", s.len, pos, from, to);
  return str_slice(s, from, to);
}


static Chars str_src_loc_str(Str src, Str path, Int pos, Int len, Int line_num, Int col,
                             Chars msg) {
  // get source line.
  // caller is responsible for raw_dealloc of return Chars.
  Str line = str_line_at_pos(src, pos);
  CharsC nl = str_ends_with_char(line, '\n') ? "" : "\n";
  // create underline.
  Char under[len_buffer] = {}; // zeroes all elements.
  if (line.len < len_buffer) { // draw underline
    for_in(i, col) under[i] = ' ';
    if (len > 0) {
      for_in(i, len) under[col + i] = '~';
    }
    else {
      under[col] = '^';
    }
  }
  // create result.
  Chars s;
  Int s_len = asprintf(&s, "%s:%ld:%ld: %s\n%*s%s%s\n",
                       path.chars, line_num + 1, col + 1, msg, FMT_STR(line), nl, under);
  counter_inc(ci_Chars); // matches asprinf.
  check(s_len > 0, "str_src_loc_str allocation failed: %s", msg);
  return s;
}

