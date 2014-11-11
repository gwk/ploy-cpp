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

#include "07-obj.h"


static void write_repr(CFile f, Obj o);

static void fmt_list_to_file(CFile f, Chars fmt, va_list args_list) {
  Chars pf = fmt;
  Char c;
  while ((c = *pf++)) {
    if (c == '%') {
      c = *pf++;
      Obj arg = va_arg(args_list, Obj);
      switch (c) {
        case 'o': write_repr(f, arg); break;
        case 'i': fprintf(f, "%li", arg.i); break;
        case 'u': fprintf(f, "%lu", arg.u); break;
        case 'p': fprintf(f, "%p", arg.r); break;
        case 's': fputs(arg.chars, f); break;
        case 'c': fputs(char_repr((Char)arg.u), f); break;
        default: error("obj_fmt: invalid placeholder: '%c'; %s", (Uns)c, fmt);
      }
    } else {
      fputc(c, f);
    }
  }
}


static void fmt_to_file(CFile f, Chars fmt, ...) {
  va_list args_list;
  va_start(args_list, fmt);
  fmt_list_to_file(f, fmt, args_list);
  va_end(args_list);
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


#if OPTION_DEALLOC_PRESERVE
#define fmt_obj_dealloc_preserve "%o"
#else
#define fmt_obj_dealloc_preserve "%p"
#endif

