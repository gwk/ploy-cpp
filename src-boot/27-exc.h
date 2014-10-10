// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// exceptions.

#include "26-parse.h"


typedef struct _Trace {
  Obj code;
  Int elided_step_count;
  struct _Trace* next;
} Trace; // Trace objects are live on the stack only.


static NO_RETURN _exc_raise(Trace* trace, Obj env, Chars_const fmt, Chars_const args_str, ...) {
  // raise an exception.
  // NOTE: there is not yet any exception unwind mechanism, so this just calls exit.
  va_list args_list;
  va_start(args_list, args_str);
  fmt_list_to_file(stderr, fmt, args_str, args_list);
  va_end(args_list);
  errL("\ntrace:");
  while (trace) {
    errFL("  %o", trace->code);
    if (trace->elided_step_count > 0) { // tail 
      errFL("  … %i", trace->elided_step_count);
    }
    trace = trace->next;
  }
  fail();
}
