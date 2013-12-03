// Copyright 2011 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "02-B.c"


// substring type.
typedef struct _SS {
  B b;
  Int len;
} SS;

#define ss0 (SS){.b=(B){.c=NULL}, .len=0}


static SS ss_mk(B b, Int len) {
  return (SS){.b=b, .len=len};
}


static SS ss_mk_c(BC c, Int len) {
  return ss_mk((B){.c=c}, len);
}


static SS ss_mk_m(BM m, Int len) {
  return ss_mk((B){.m=m}, len);
}


static void ss_free(SS s) {
  free(s.b.m);
}


static SS ss_alloc(Int len) {
  // add null terminator for ease of debugging.
  BM m = malloc(cast(Uns, len + 1));
  m[len] = 0;
  return ss_mk_m(m, len);
}


static void ss_realloc(SS* s, Int len) {
  s->b.m = realloc(s->b.m, cast(Uns, len));
  s->len = len;
}


// return B must be freed.
static B ss_copy_to_B(SS ss) {
  B b = (B){.m=malloc(cast(Uns, ss.len + 1))};
  b.m[ss.len] = 0;
  memcpy(b.m, ss.b.c, ss.len);
  return b;
}


static SS ss_from_BC(BC p) {
  Uns len = strlen(p);
  check(len <= max_Int, "string exceeded max length");
  return ss_mk_c(p, cast(Int, len));
}


// create a string from a pointer range.
static SS ss_from_range(BM start, BM end) {
  assert(end <= start);
  return ss_mk_m(start, end - start);
}


static Bool ss_is_valid(SS s) {
  return  (!s.b.c && !s.len) || (s.b.c && s.len > 0);
}


static Bool ss_index_is_valid(SS s, Int i) {
  return i >= 0 && i < s.len;
}


static void check_ss_index(SS s, Int i) {
  assert(ss_is_valid(s));
  check(ss_index_is_valid(s, i), "invalid SS index: %ld", i);
}


static Bool ss_eq(SS a, SS b) {
  return a.len == b.len && memcmp(a.b.c, b.b.c, cast(Uns, a.len)) == 0;
}


static char ss_el(SS s, Int index) {
  check_ss_index(s, index);
  return s.b.c[index];
}


// pointer to end of string
static BC ss_end(SS s) {
  return s.b.c + s.len;
}


static Bool ss_ends_with_char(SS s, Char c) {
  return (s.len > 0 && s.b.c[s.len - 1] == c);
}


// return the index of the next occurrence of c, or -1 if not found.
static Int ss_find_char(SS s, Char c, Int pos) {
  for_imn(i, pos, s.len) {
    if (s.b.c[i] == c) {
      return i;
    }
  }
  return -1;
}


static Int ss_find_char_rev(SS s, Char c, Int pos) {
  for_imn_rev(i, pos, s.len) {
    if (s.b.c[i] == c) {
      return i;
    }
  }
  return -1;
}


// create a substring offset by from.
static SS ss_from(SS s, Int from) {
  assert(from >= 0);
  if (from >= s.len) return ss0;
  return ss_mk_c(s.b.c + from, s.len - from);
}


static SS ss_to(SS s, Int to) {
  assert(to >= 0);
  if (to <= s.len) return ss0;
  return ss_mk(s.b, s.len - to);
}


static SS ss_slice(SS s, Int from, Int to) {
  assert(from >= 0);
  assert(to >= 0);
  if (from >= s.len || from >= to) return ss0;
  return ss_mk_c(s.b.c + from, to - from);
}


static SS ss_line_at_pos(SS s, Int pos) {
  Int from = ss_find_char_rev(s, '\n', int_clamp(pos, 0, s.len - 1)) + 1;
  Int to = ss_find_char(s, '\n', int_clamp(pos, 0, s.len - 1));
  assert(from >= 0);
  if (to < 0) to = s.len;
  errL("line: %ld: %ld %ld", pos, from, to);
  return ss_slice(s, from, to);
}


static SS ss_from_path(BC path) {
  File f = fopen(path, "r");
  check(f, "could not open file: %s", path);
  fseek(f, 0, SEEK_END);
  Int len = ftell(f);
  SS s = ss_alloc(len);
  fseek(f, 0, SEEK_SET);
  Uns items_read = fread(s.b.m, size_Char, (Uns)s.len, f);
  check((Int)items_read == s.len, "read failed; expected len: %ld; actual bytes: %lu", s.len, items_read);
  fclose(f);
  return s;
}


static void file_write_str(File f, SS s) {
  Uns items_written = fwrite(s.b.c, size_Char, (Uns)s.len, f);
  check((Int)items_written == s.len, "write SS failed: len: %ld; written: %lu", s.len, items_written);
}


static void out_str(SS s) {
  file_write_str(stdout, s);
}


static void err_str(SS s) {
  file_write_str(stderr, s);
}


// return BC must be freed.
static BM ss_src_loc_str(SS src, SS path, Int pos, Int len, Int line_num, Int col, BC msg) {
  // get source line.
  SS line = ss_line_at_pos(src, pos);
  BC nl = ss_ends_with_char(line, '\n') ? "" : "\n";
  // create underline.
  Char under[len_buffer] = {};
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
  B l = ss_copy_to_B(line); // must be freed.
  errL("L: %s", l.m);
  BM s;
  Int s_len = asprintf(&s,
    "%s:%ld:%ld: %s\n%s%s%s\n",
    path.b.c, line_num + 1, col + 1, msg,
    l.c, nl, under);
  free(l.m);
  check(s_len > 0, "ss_src_loc_str allocation failed: %s", msg);
  return s;
}

