// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// string formatting.

// the format syntax is similar to printf, but simplified and extended to handle ploy objects.
// %o: Obj.
// %i: Int.
// %u: Uns.
// %p: Raw.
// %s: Chars.
// %c: Char.

#include "21-write-repr.h"


#define FMT1(T, c_exp, ...) \
static void _fmt1(CFile f, Chars fmt, T item) { \
  if (*fmt != c_exp) { \
    fprintf(stderr, "formatter '%c' received %s; fmt: '%s'", *fmt, #T, fmt); \
    fflush(stderr); \
    assert(0); \
    exit(1); \
  } \
  __VA_ARGS__; \
} \

FMT1(Obj, 'o', write_repr(f, item));
FMT1(Int, 'i', fprintf(f, "%li", item));
FMT1(Uns, 'u', fprintf(f, "%lu", item));
FMT1(Raw, 'p', fprintf(f, "%p", item));
UNUSED FMT1(Char, 'c', fprintf(f, "%c", item));
FMT1(Chars, 's', fprintf(f, "%s", item));

#undef FMT1


static Chars _fmt_adv(CFile f, Chars fmt) {
  Char c;
  while ((c = *fmt++)) {
    if (c == '%') {
      if (*fmt == '%') {
        fputc('%', f);
        fmt++;
      } else {
        return fmt;
      }
    } else {
      fputc(c, f);
    }
  }
  return null;
}


template <typename... Ts>
static void fmt_to_file(CFile f, Chars fmt) {
  fmt = _fmt_adv(f, fmt);
  check(!fmt, "missing format arguments: '%s'", fmt);
}

template <typename T, typename... Ts>
static void fmt_to_file(CFile f, Chars fmt, T item, Ts... items) {
  fmt = _fmt_adv(f, fmt);
  check(fmt, "excessive format arguments");
  _fmt1(f, fmt, item);
  fmt_to_file(f, ++fmt, items...);
}


Obj dbg(Obj o); // not declared static so that it is always available in debugger.
Obj dbg(Obj o) {
  if (o.is_ref()) {
    errFL("%p : %o", o.r, o);
  } else {
    errFL("%o", o);
  }
  err_flush();
  return o;
}
