// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// exceptions.

#include "23-parse.h"


static NO_RETURN _exc_raise(Obj env, Chars_const fmt, Chars_const args_str, ...) {
  // raise an exception.
  // NOTE: there is not yet any exception unwind mechanism, so this just calls exit.
  va_list args_list;
  va_start(args_list, args_str);
  fmt_list_to_file(stderr, fmt, args_str, args_list);
  va_end(args_list);
  err_nl();
  env_trace(env, false);
  exit(1);
}
