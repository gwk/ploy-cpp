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


static Dict_src_locs global_src_locs;

static Obj track_src(Obj original, Obj derived) {
  if (!original.is_ref() || !derived.is_ref()) return derived;
  auto lookup = global_src_locs.find(original);
  if (lookup != global_src_locs.end()) {
    Obj lo = lookup->second;
    assert(lo.vld());
    Obj& ld = global_src_locs[derived];
    if (!ld.vld()) { // an expression may be aliased by expansion and therefore already present.
      derived.ret();
      ld = lo.ret();
    }
  }
  return derived;
}


[[noreturn]] static void _exc_raise(Trace* trace) {
  // raise an exception.
  // NOTE: there is not yet any exception unwind mechanism, so this just calls exit.
  errL("\ntrace:");
  while (trace) {
    auto lookup = global_src_locs.find(trace->code);
    if (lookup == global_src_locs.end()) {
      errFL("  %o", trace->code);
    } else {
      Obj loc = lookup->second;
      assert(loc.vld());
      Obj path = cmpd_el(loc, 0);
      Obj src = cmpd_el(loc, 1);
      Obj pos = cmpd_el(loc, 2);
      Obj len = cmpd_el(loc, 3);
      Obj line = cmpd_el(loc, 4);
      Obj col = cmpd_el(loc, 5);
      CharsM msg = str_src_loc_str(data_str(path), data_str(src),
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
