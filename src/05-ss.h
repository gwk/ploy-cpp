// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "04-chars.h"


typedef struct _SS {
  Int len;
  Chars chars;
} SS; // Substring.

#define ss0 (SS){.len=0, .chars=(Chars){.c=NULL}}

// for use with "%*s" formatter.
#define FMT_SS(ss) cast(I32, (ss).len), (ss).chars.c


static SS ss_mk(Int len, Chars chars) {
  return (SS){.len=len, .chars=chars};
}


static SS ss_mk_c(Int len, CharsC c) {
  return ss_mk(len, (Chars){.c=c});
}


static SS ss_mk_m(Int len, CharsM m) {
  return ss_mk(len, (Chars){.m=m});
}


static void ss_dealloc(SS s) {
  raw_dealloc(s.chars.m, ci_Str);
}


static SS ss_alloc(Int len) {
  // add null terminator for easier debugging.
  CharsM m = raw_alloc(len + 1, ci_Str);
  m[len] = 0;
  return ss_mk_m(len, m);
}


static void ss_realloc(SS* s, Int len) {
  s->chars.m = realloc(s->chars.m, cast(Uns, len));
  s->len = len;
}


// return B must be freed.
static Chars ss_copy_to_B(SS ss) {
  Chars b = (Chars){.m=raw_alloc(ss.len + 1, ci_Chars)};
  b.m[ss.len] = 0;
  memcpy(b.m, ss.chars.c, ss.len);
  return b;
}


static SS ss_from_BC(CharsC p) {
  Uns len = strnlen(p, max_Int);
  check(len <= max_Int, "string exceeded max length");
  return ss_mk_c(cast(Int, len), p);
}


// create a string from a pointer range.
UNUSED_FN static SS ss_from_range(CharsM start, CharsM end) {
  assert(end <= start);
  return ss_mk_m(end - start, start);
}


static Bool ss_is_valid(SS s) {
  return  (!s.chars.c && !s.len) || (s.chars.c && s.len > 0);
}


static Bool ss_index_is_valid(SS s, Int i) {
  return i >= 0 && i < s.len;
}


static void check_ss_index(SS s, Int i) {
  assert(ss_is_valid(s));
  check(ss_index_is_valid(s, i), "invalid SS index: %ld", i);
}


static Bool ss_eq(SS a, SS b) {
  return a.len == b.len && memcmp(a.chars.c, b.chars.c, cast(Uns, a.len)) == 0;
}


UNUSED_FN static Char ss_el(SS s, Int index) {
  check_ss_index(s, index);
  return s.chars.c[index];
}


// pointer to end of string
UNUSED_FN static CharsC ss_end(SS s) {
  return s.chars.c + s.len;
}


static Bool ss_ends_with_char(SS s, Char c) {
  return (s.len > 0 && s.chars.c[s.len - 1] == c);
}


// create a substring offset by from.
UNUSED_FN static SS ss_from(SS s, Int from) {
  assert(from >= 0);
  if (from >= s.len) return ss0;
  return ss_mk_c(s.len - from, s.chars.c + from);
}


UNUSED_FN static SS ss_to(SS s, Int to) {
  assert(to >= 0);
  if (to <= s.len) return ss0;
  return ss_mk(s.len - to, s.chars);
}


static SS ss_slice(SS s, Int from, Int to) {
  assert(from >= 0);
  assert(to >= 0);
  if (from >= s.len || from >= to) return ss0;
  return ss_mk_c(to - from, s.chars.c + from);
}


static Int ss_find_line_start(SS s, Int pos) {
  for_in_rev(i, pos - 1) {
    //errFL("ss_find_line_start: pos:%ld i:%ld", pos, i);
    if (s.chars.c[i] == '\n') {
      return i + 1;
    }
  }
  return 0;
}


static Int ss_find_line_end(SS s, Int pos) {
  for_imn(i, pos, s.len) {
    if (s.chars.c[i] == '\n') {
      return i;
    }
  }
  return s.len;
}


static SS ss_line_at_pos(SS s, Int pos) {
  Int from = ss_find_line_start(s, pos);
  Int to = ss_find_line_end(s, pos);
  assert(from >= 0);
  if (to < 0) to = s.len;
  //errFL("ss_line_at_pos: len:%ld pos:%ld from:%ld to:%ld", s.len, pos, from, to);
  return ss_slice(s, from, to);
}


// return BC must be freed.
static CharsM ss_src_loc_str(SS src, SS path, Int pos, Int len, Int line_num, Int col, CharsC msg) {
  // get source line.
  SS line = ss_line_at_pos(src, pos);
  CharsC nl = ss_ends_with_char(line, '\n') ? "" : "\n";
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
  Chars l = ss_copy_to_B(line); // must be freed.
  CharsM s;
  Int s_len = asprintf(&s,
    "%s:%ld:%ld: %s\n%s%s%s\n",
    path.chars.c, line_num + 1, col + 1, msg, l.c, nl, under);
  raw_dealloc(l.m, ci_Chars);
  check(s_len > 0, "ss_src_loc_str allocation failed: %s", msg);
  return s;
}

