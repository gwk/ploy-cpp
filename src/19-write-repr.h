// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-host.h"


static void write_data(CFile f, Obj d) {
  assert(ref_is_data(d));
  Chars p = data_ptr(d);
  fwrite(p, 1, cast(Uns, data_len(d)), f);
}


static void write_repr_data(CFile f, Obj d) {
  assert(ref_is_data(d));
  Chars p = data_ptr(d);
  fputc('\'', f);
  for_in(i, data_len(d)) {
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


static void write_repr_obj(CFile f, Obj o, Set* s);


static void write_repr_vec_vec(CFile f, Obj v, Set* s) {
  assert(ref_is_vec(v));
  Mem m = vec_ref_mem(v);
  fputs("[", f);
  for_in(i, m.len) {
    if (i) fputc(' ', f);
    write_repr_obj(f, m.els[i], s);
  }
  fputs("]", f);
}


static void write_repr_chain(CFile f, Obj c, Set* s) {
  assert(ref_is_vec(c));
  fputs("[:", f);
  Bool first = true;
  loop {
    if (first) first = false;
    else fputc(' ', f);
    write_repr_obj(f, chain_hd(c), s);
    Obj tl = chain_tl(c);
    if (tl.u == END.u) break;
    assert(obj_is_vec_ref(tl));
    c = tl;
  }
  fputs("]", f);
}


static void write_repr_fat_chain(CFile f, Obj c, Set* s) {
  assert(ref_is_vec(c));
  fputs("[", f);
  loop {
    Mem m = vec_ref_mem(c);
    fputs("|", f);
    for_in(i, m.len - 1) {
      if (i) fputc(' ', f);
      write_repr_obj(f, m.els[i], s);
    }
    Obj tl = m.els[m.len - 1];
    if (tl.u == END.u) break;
    assert(obj_is_vec_ref(tl));
    c = tl;
  }
  fputs("]", f);
}


static void write_repr_par(CFile f, Obj p, Set* s, Char c) {
  fputc('`', f);
  fputc(c, f);
  assert(vec_ref_len(p) == 4);
  Obj* els = vec_ref_els(p);
  Obj name = els[1];
  Obj type = els[2];
  Obj expr = els[3];
  assert(obj_is_symbol(name));
  Obj d = sym_data(name);
  write_data(f, d);
  if (type.u != NIL.u) {
    fputc(':', f);
    write_repr_obj(f, type, s); // TODO: quote this?
  }
  if (expr.u != NIL.u) {
    fputc('=', f);
    write_repr_obj(f, expr, s); // TODO: quote this?
  }
}


static void write_repr_vec(CFile f, Obj v, Set* s) {
  assert(ref_is_vec(v));
  #if 0 // TODO: incomplete mess.
  if (false) {
    write_repr_vec_quoted(f, v, s);
    return;
  }
  #endif
  switch (vec_ref_shape(v)) {
    case vs_vec:          write_repr_vec_vec(f, v, s);    return;
    case vs_chain:        write_repr_chain(f, v, s);      return;
    case vs_chain_blocks: write_repr_fat_chain(f, v, s);  return;
    case vs_label:        write_repr_par(f, v, s, '-');   return;
    case vs_variad:       write_repr_par(f, v, s, '&');   return;
  }
  assert(0);
}


static void write_repr_Func_host(CFile f, Obj func) {
  fputs("(Func-host ", f);
  Func_host* fh = ref_body(func);
  Obj d = sym_data(fh->sym);
  write_data(f, d);
  fputc(')', f);
}


static void write_repr_obj(CFile f, Obj o, Set* s) {
  Obj_tag ot = obj_tag(o);
  if (ot == ot_int) {
    fprintf(f, "%ld", int_val(o));
  }
  else if (ot == ot_sym) {
    if (o.u == VEC0.u) {
      fputs("[]", f);
    }
    else if (o.u == CHAIN0.u) {
      fputs("[|]", f);
    }
    else {
      fputc('`', f);
      write_data(f, sym_data(o));
    }
  }
  else if (ot == ot_data) { // data-word
    // TODO: support all word values.
    assert(o.u == blank.u);
    fputs("''", f);
  }
  else {
    if (set_contains(s, o)) { // recursed
      fputs("↺", f); // anticlockwise gapped circle arrow
    }
    else {
      set_insert(s, o);
      switch (ref_struct_tag(o)) {
        case st_Data: write_repr_data(f, o); break;
        case st_Vec:  write_repr_vec(f, o, s); break;
        case st_I32:          fputs("(I32 ?)", f); break;
        case st_I64:          fputs("(I64 ?)", f); break;
        case st_U32:          fputs("(U32 ?)", f); break;
        case st_U64:          fputs("(U64 ?)", f); break;
        case st_F32:          fputs("(F32 ?)", f); break;
        case st_F64:          fputs("(F64 ?)", f); break;
        case st_File:         fprintf(f, "(File %p)", file_file(o)); break;
        case st_Func_host:    write_repr_Func_host(f, o); break;
        case st_Reserved_A:
        case st_Reserved_B:
        case st_Reserved_C:
        case st_Reserved_D:
        case st_Reserved_E:
        case st_Reserved_F: fputs("(ReservedX)", f); break;
      }
      set_remove(s, o);
    }
  }
  err_flush(); // TODO: for debugging only?
}


static void write_repr(CFile f, Obj o) {
  Set s = set0;
  write_repr_obj(f, o, &s);
  set_dealloc(&s);
}
