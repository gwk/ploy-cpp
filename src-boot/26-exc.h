// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// exceptions.

#include "25-host.h"


struct Trace {
  Obj code;
  Int elided_step_count;
  Trace* next;
  Trace(Obj c, Int e, Trace* n): code(c), elided_step_count(e), next(n) {}
}; // Trace objects are live on the stack only.


static Dict global_src_locs;

static Obj track_src(Obj original, Obj derived) {
  if (!original.is_ref() || !derived.is_ref()) return derived;
  Obj src_loc = global_src_locs.fetch(original);
  if (src_loc.vld() && !global_src_locs.contains(derived)) {
    global_src_locs.insert(derived.ret(), src_loc.ret());
  }
  return derived;
}


[[noreturn]] static void _exc_raise(Trace* trace) {
  // raise an exception.
  // NOTE: there is not yet any exception unwind mechanism, so this just calls exit.
  errL("\ntrace:");
  while (trace) {
    Obj loc = global_src_locs.fetch(trace->code);
    if (!loc.vld()) {
      errFL("  %o", trace->code);
    } else {
      Obj path = loc.cmpd_el(0);
      Obj src = loc.cmpd_el(1);
      Obj pos = loc.cmpd_el(2);
      Obj len = loc.cmpd_el(3);
      Obj line = loc.cmpd_el(4);
      Obj col = loc.cmpd_el(5);
      CharsM msg = str_src_loc_str(path.data_str(), src.data_str(),
        pos.int_val(), len.int_val(), line.int_val(), col.int_val(), "");
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
