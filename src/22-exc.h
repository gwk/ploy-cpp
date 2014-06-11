// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// exceptions.

#include "21-env.h"


static NO_RETURN _exc_raise(Obj env, Chars fmt, Chars args_src, ...) {
  Chars pa = args_src;
  Int arg_count = bit(*pa);
  while (*pa) {
    if (*pa++ == ',') {
      arg_count++;
    }
  }
  Int i = 0;
  va_list arg_list;
  va_start(arg_list, args_src);
  Chars pf = fmt;
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
    }
    else {
      fputc(c, stderr);
    }
  }
  va_end(arg_list);
  err_nl();
  env_trace(env);
  check(i == arg_count, "only %li items in exc_raise format: '%s'; %li args: '%s'",
    i, fmt, arg_count, args_src);
  exit(1);
}

// these macros expect env:Obj to be defined in the current scope.
#define exc_raise(fmt, ...) _exc_raise(env, fmt, #__VA_ARGS__, ##__VA_ARGS__)
#define exc_check(condition, ...) if (!(condition)) exc_raise(__VA_ARGS__)