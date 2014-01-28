// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "17-bool.h"


static void write_data(File f, Obj d) {
  assert(ref_is_data(d));
  BC p = data_ptr(d).c;
  fwrite(p, 1, cast(Uns, ref_len(d)), f);
}



static void write_repr_data(File f, Obj d) {
  assert(ref_is_data(d));
  BC p = data_ptr(d).c;
  fputc('\'', f);
  for_in(i, ref_len(d)) {
    Char c = p[i];
    switch (c) {
      case '\a': fputc('\\', f); fputc('a', f);  continue; // bell - BEL
      case '\b': fputc('\\', f); fputc('b', f);  continue; // backspace - BS
      case '\f': fputc('\\', f); fputc('f', f);  continue; // form feed - FF
      case '\n': fputc('\\', f); fputc('n', f);  continue; // line feed - LF
      case '\r': fputc('\\', f); fputc('r', f);  continue; // carriage return - CR
      case '\t': fputc('\\', f); fputc('t', f);  continue; // horizontal tab - TAB
      case '\v': fputc('\\', f); fputc('v', f);  continue; // vertical tab - VT
      case '\\': fputc('\\', f); fputc('\\', f); continue;
      case '\'': fputc('\\', f); fputc('\'', f); continue;
      case '"':  fputc('\\', f); fputc('"', f);  continue;
    }
    // TODO: escape non-printable characters
    fputc(c, f);
  }
  fputc('\'', f);
}


static void write_repr_obj(File f, Obj o);


static void write_repr_vec_vec(File f, Obj v) {
  assert(ref_is_vec(v));
  Int len = ref_len(v);
  Obj* els = vec_els(v);
  fputs("{", f);
  for_in(i, len) {
    if (i) fputs(" ", f);
    write_repr_obj(f, els[i]);
  }
  fputs("}", f);
}


static void write_repr_chain(File f, Obj c) {
  assert(ref_is_vec(c));
  fputs("[", f);
  Bool first = true;
  loop {
    if (first) first = false;
    else fputs(" ", f);
    write_repr_obj(f, chain_hd(c));
    Obj tl = chain_tl(c);
    if (tl.u == END.u) break;
    assert(obj_is_vec(tl));
    c = tl;
  }
  fputs("]", f);
}


static void write_repr_chain_blocks(File f, Obj c) {
  assert(ref_is_vec(c));
  fputs("[", f);
  loop {
    Obj* els = vec_els(c);
    Int len = ref_len(c);
    fputs("|", f);
    for_imn(i, 1, len) { // note: unlike lisp, tl is in position 0.
      if (i) fputs(" ", f);
      write_repr_obj(f, els[i]);
    }
    Obj tl = els[0];
    if (tl.u == END.u) break;
    assert(obj_is_vec(tl));
    c = tl;
  }
  fputs("]", f);
}


static void write_repr_vec(File f, Obj v) {
  assert(ref_is_vec(v));
  switch (vec_shape(v)) {
    case vs_vec:          write_repr_vec_vec(f, v);       return;
    case vs_chain:        write_repr_chain(f, v);         return;
    case vs_chain_blocks: write_repr_chain_blocks(f, v);  return;
  }
  assert(0);
}


static void write_repr_Func_host(File f, Obj func, Int i) {
  fprintf(f, "(Func-host-%ld ", i);
  Func_host* fh = ref_body(func);
  Obj d = sym_data(fh->sym);
  write_data(f, d);
  fputc(')', f);
}


static void write_repr_obj(File f, Obj o) {
  Obj_tag ot = obj_tag(o);
  if (ot & ot_flt_bit) {
    fprintf(f, "(Flt %f)", flt_val(o));
  }
  else if (ot == ot_int) {
    fprintf(f, "%ld", int_val(o));
  }
  else if (ot == ot_sym) {
    if (o.u == VEC0.u) {
      fputs("{}", f);
    }
    else if (o.u == CHAIN0.u) {
      fputs("[]", f);
    }
    else {
      write_data(f, sym_data(o));
    }
  }
  else if (ot == ot_data) { // data-word
    // TODO: support all word values.
    assert(o.u == blank.u);
    fputs("''", f);
  }
  else {
    switch (ref_struct_tag(o)) {
      case st_Data: write_repr_data(f, o); break;
      case st_Vec:  write_repr_vec(f, o); break;
      case st_I32:          fputs("(I32 ?)", f); break;
      case st_I64:          fputs("(I64 ?)", f); break;
      case st_U32:          fputs("(U32 ?)", f); break;
      case st_U64:          fputs("(U64 ?)", f); break;
      case st_F32:          fputs("(F32 ?)", f); break;
      case st_F64:          fputs("(F64 ?)", f); break;
      case st_File:         fputs("(File ?)", f); break;
      case st_Func_host_1:  write_repr_Func_host(f, o, 1); break;
      case st_Func_host_2:  write_repr_Func_host(f, o, 2); break;
      case st_Func_host_3:  write_repr_Func_host(f, o, 3); break;
      case st_Reserved_A:
      case st_Reserved_B:
      case st_Reserved_C: fputs("(ReservedX)", f); break;
      case st_DEALLOC: error("deallocated object is still referenced: %p", o.p);
    }
  }
  err_flush(); // TODO: for debugging only?
}
