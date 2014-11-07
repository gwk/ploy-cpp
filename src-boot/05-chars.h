// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// null-terminated c string and file types.
// in general we ignore const correctness, casting const strings to Chars immediately.

#include "04-raw.h"


static Bool chars_eq(Chars_const a, Chars_const b) {
  return strcmp(a, b) == 0;
}


// get the base name of the path argument.
UNUSED_FN static Chars_const chars_path_base(Chars_const path) {
  Int offset = 0;
  Int i = 0;
  loop {
    if (!path[i]) break; // EOS
    if (path[i] == '/') offset = i + 1;
    i += 1;
  }
  return path + offset;
}


static Chars char_repr(Char c) {
  static Char reprs[256][8] = {};
  Chars r = reprs[cast(Int, c)];
  if (*r) return r;
  switch (c) {
    case '\a': strcpy(r, "\\a");   break; // bell - BEL
    case '\b': strcpy(r, "\\b");   break; // backspace - BS
    case '\f': strcpy(r, "\\f");   break; // form feed - FF
    case '\n': strcpy(r, "\\n");   break; // line feed - LF
    case '\r': strcpy(r, "\\r");   break; // carriage return - CR
    case '\t': strcpy(r, "\\t");   break; // horizontal tab - TAB
    case '\v': strcpy(r, "\\v");   break; // vertical tab - VT
    case '\\': strcpy(r, "\\\\");  break;
    case '\'': strcpy(r, "\\'");   break;
    case '"':  strcpy(r, "\\\"");  break;
    default:
    if (isprint(c)) {
      r[0] = c;
      r[1] = '\0';
    } else {
      sprintf(r, "\\x%02x", c);
    }
  }
  return r;
}


static void chars_dealloc(Chars c) {
  raw_dealloc(c, ci_Chars);
}


static Chars chars_alloc(Int len) {
  return cast(Chars, raw_alloc(len, ci_Chars));
}


static Int chars_append(Chars* p, Int* cap, Int len, Char c) {
  // returns updated len.
  assert(len <= *cap);
  if (len == *cap) {
    assert(*cap > 0);
    *cap *= 2;
    *p = cast(Chars, raw_realloc(*p, *cap, ci_Chars));
  }
  Chars chars = *p;
  chars[len] = c;
  return len + 1;
}

