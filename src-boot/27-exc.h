// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// exceptions.

#include "26-parse.h"


struct Trace {
  Obj code;
  Int elided_step_count;
  Trace* next;
}; // Trace objects are live on the stack only.


static Dict global_src_locs;

static Obj track_src(Obj original, Obj derived) {
  if (!obj_is_ref(original) || !obj_is_ref(derived)) return derived;
  Obj src_loc = dict_fetch(&global_src_locs, original);
  if (!is(src_loc, obj0) && !dict_contains(&global_src_locs, derived)) {
    dict_insert(&global_src_locs, rc_ret(derived), rc_ret(src_loc));
  }
  return derived;
}


static NO_RETURN _exc_raise(Trace* trace, Obj env, Chars_const fmt, Chars_const args_str, ...) {
  // raise an exception.
  // NOTE: there is not yet any exception unwind mechanism, so this just calls exit.
  va_list args_list;
  va_start(args_list, args_str);
  fmt_list_to_file(stderr, fmt, args_str, args_list);
  va_end(args_list);
  errL("\ntrace:");
  while (trace) {
    Obj loc = dict_fetch(&global_src_locs, trace->code);
    if (is(loc, obj0)) {
      errFL("  %o", trace->code);
    } else {
      Obj path = cmpd_el(loc, 0);
      Obj src = cmpd_el(loc, 1);
      Obj pos = cmpd_el(loc, 2);
      Obj len = cmpd_el(loc, 3);
      Obj line = cmpd_el(loc, 4);
      Obj col = cmpd_el(loc, 5);
      Chars msg = str_src_loc_str(data_str(path), data_str(src),
        int_val(pos), int_val(len), int_val(line), int_val(col), "");
      fputs("  ", stderr);
      fputs(msg, stderr);
      raw_dealloc(msg, ci_Chars);
    }
    if (trace->elided_step_count > 0) { // tail 
      errFL("  … %i", trace->elided_step_count);
    }
    trace = trace->next;
  }
  fail();
}
