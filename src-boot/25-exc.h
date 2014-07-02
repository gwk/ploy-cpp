// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// exceptions.

#include "24-parse.h"


static NO_RETURN _exc_raise(Obj env, Chars_const fmt, Chars_const args_src, ...) {
  // raise an exception.
  // NOTE: there is not yet any exception unwind mechanism, so this just calls exit.
  Chars_const pa = args_src;
  Int arg_count = bit(*pa);
  while (*pa) {
    if (*pa++ == ',') {
      arg_count++;
    }
  }
  Int i = 0;
  va_list arg_list;
  va_start(arg_list, args_src);
  Chars_const pf = fmt;
  Char c;
  while ((c = *pf++)) {
    if (c == '%') {
      check(i < arg_count, "more than %li items in exc_raise format: '%s'; args: '%s'",
        arg_count, fmt, args_src);
      i++;
      c = *pf++;
      Obj arg = va_arg(arg_list, Obj);
      switch (c) {
        case 'o': write_repr(stderr, arg); break;
        case 'i': errF("%li", arg.i); break;
        case 's': errF("%s", arg.c); break;
        default: error("exc_raise: invalid placeholder: '%c'; %s", c, fmt);
      }
    } else {
      fputc(c, stderr);
    }
  }
  va_end(arg_list);
  err_nl();
  env_trace(env, false);
  check(i == arg_count, "only %li items in exc_raise format: '%s'; %li args: '%s'",
    i, fmt, arg_count, args_src);
  exit(1);
}
