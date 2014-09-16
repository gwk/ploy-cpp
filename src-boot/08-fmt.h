// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// string formatting.

#include "07-obj.h"


static void write_repr(CFile f, Obj o);

static void fmt_list_to_file(CFile f, Chars_const fmt, Chars_const args_str, va_list args_list) {
  // the format syntax is similar to printf, but simplified and extended to handle ploy objects.
  // %o: Obj.
  // %i: Int.
  // %u: Uns.
  // %p: Raw.
  // %s: Chars.
  // %c: Char. NOTE: Char arguments must be cast to Uns in the call to align variadic args.
  // for safety and debug convenience, the function takes an additional string argument,
  // which should be provided by a wrapper macro that passes the format #__VA_ARGS__.
  // this facilitates a runtime check that the format matches the argument count.
  Chars_const pa = args_str;
  Int arg_count = bit(*pa);
  Int level = 0;
  while (*pa) {
    switch (*pa++) {
      case '(': level++; continue;
      case ')': level--; continue;
      case ',': if (!level) arg_count++; continue;
    }
  }
  Int i = 0;
  Chars_const pf = fmt;
  Char c;
  while ((c = *pf++)) {
    if (c == '%') {
      check(i < arg_count, "obj_fmt: more than %i items in format: '%s'; args: '%s'",
        arg_count, fmt, args_str);
      i++;
      c = *pf++;
      Obj arg = va_arg(args_list, Obj);
      switch (c) {
        case 'o': write_repr(f, arg); break;
        case 'i': fprintf(f, "%li", arg.i); break;
        case 'u': fprintf(f, "%lu", arg.u); break;
        case 'p': fprintf(f, "%p", arg.r); break;
        case 's': fputs(arg.c, f); break;
        case 'c': fputs(char_repr((Char)arg.u), f); break;
        default: error("obj_fmt: invalid placeholder: '%c'; %s", (Uns)c, fmt);
      }
    } else {
      fputc(c, f);
    }
  }
  check(i == arg_count, "only %i items in format: '%s'; %i args: '%s'",
    i, fmt, arg_count, args_str);
}


static void fmt_to_file(CFile f, Chars_const fmt, Chars_const args_str, ...) {
  va_list args_list;
  va_start(args_list, args_str);
  fmt_list_to_file(f, fmt, args_str, args_list);
  va_end(args_list);
}


Obj dbg(Obj o); // not declared static so that it is always available in debugger.
Obj dbg(Obj o) {
  if (obj_is_ref(o)) {
    errFL("%p : %o", o.r, o);
  } else {
    errFL("%o", o);
  }
  err_flush();
  return o;
}


#if OPT_DEALLOC_PRESERVE
#define fmt_obj_dealloc_preserve "%o"
#else
#define fmt_obj_dealloc_preserve "%p"
#endif

