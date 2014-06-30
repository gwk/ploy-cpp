// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "20-host.h"


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


static void write_repr_sym(CFile f, Obj s, Bool is_quoted) {
  assert(obj_is_sym(s));
  if (!is_quoted && !sym_is_special(s)) {
    fputc('`', f);
  }
  Obj d = sym_data(s);
  write_data(f, d);
}


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Set* s);

static void write_repr_vec_vec(CFile f, Obj v, Bool is_quoted, Set* s) {
  assert(ref_is_vec(v));
  Mem m = vec_ref_mem(v);
  if (is_quoted) fputs("???", f);
  fputc('[', f);
  for_in(i, m.len) {
    if (i) fputc(' ', f);
    write_repr_obj(f, m.els[i], is_quoted, s);
  }
  fputc(']', f);
}


static void write_repr_chain(CFile f, Obj c, Bool is_quoted, Set* s) {
  assert(ref_is_vec(c));
  if (is_quoted) fputs("???", f);
  fputs("[:", f);
  Bool first = true;
  loop {
    if (first) {
      first = false;
    } else {
      fputc(' ', f);
    }
    write_repr_obj(f, chain_hd(c), is_quoted, s);
    Obj tl = chain_tl(c);
    if (tl.u == s_END.u) break;
    assert(obj_is_vec_ref(tl));
    c = tl;
  }
  fputc(']', f);
}


static void write_repr_fat_chain(CFile f, Obj c, Bool is_quoted, Set* s) {
  assert(ref_is_vec(c));
  if (is_quoted) fputs("???", f);
  fputc('[', f);
  loop {
    Mem m = vec_ref_mem(c);
    fputc('|', f);
    for_in(i, m.len - 1) {
      if (i) fputc(' ', f);
      write_repr_obj(f, m.els[i], is_quoted, s);
    }
    Obj tl = m.els[m.len - 1];
    if (tl.u == s_END.u) break;
    assert(obj_is_vec_ref(tl));
    c = tl;
  }
  fputc(']', f);
}


static void write_repr_quo(CFile f, Obj q, Bool is_quoted, Set* s) {
  assert(vec_ref_len(q) == 2);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('`', f);
  write_repr_obj(f, vec_ref_el(q, 1), true, s);
}


static void write_repr_qua(CFile f, Obj q, Bool is_quoted, Set* s) {
  assert(vec_ref_len(q) == 2);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('~', f);
  write_repr_obj(f, vec_ref_el(q, 1), true, s);
}


static void write_repr_unq(CFile f, Obj q, Bool is_quoted, Set* s) {
  assert(vec_ref_len(q) == 2);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc(',', f);
  write_repr_obj(f, vec_ref_el(q, 1), true, s);
}


static void write_repr_par(CFile f, Obj p, Bool is_quoted, Set* s, Char c) {
  assert(vec_ref_len(p) == 4);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc(c, f);
  Obj* els = vec_ref_els(p);
  Obj name = els[1];
  Obj type = els[2];
  Obj expr = els[3];
  write_repr_obj(f, name, true, s);
  if (type.u != s_nil.u) {
    fputc(':', f);
    write_repr_obj(f, type, true, s);
  }
  if (expr.u != s_nil.u) {
    fputc('=', f);
    write_repr_obj(f, expr, true, s);
  }
}


static void write_repr_seq(CFile f, Obj v, Bool is_quoted, Set* s) {
  assert(ref_is_vec(v));
  Mem m = vec_ref_mem(v);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('[', f);
  for_imn(i, 1, m.len) {
    if (i > 1) fputc(' ', f);
    write_repr_obj(f, m.els[i], true, s);
  }
  fputc(']', f);
}


static void write_repr_vec(CFile f, Obj v, Bool is_quoted, Set* s) {
  assert(ref_is_vec(v));
  switch (vec_ref_shape(v)) {
    case vs_vec:          write_repr_vec_vec(f, v, is_quoted, s);   return;
    case vs_chain:        write_repr_chain(f, v, is_quoted, s);     return;
    case vs_chain_blocks: write_repr_fat_chain(f, v, is_quoted, s); return;
    case vs_quo:          write_repr_quo(f, v, is_quoted, s);       return;
    case vs_qua:          write_repr_qua(f, v, is_quoted, s);       return;
    case vs_unq:          write_repr_unq(f, v, is_quoted, s);       return;
    case vs_label:        write_repr_par(f, v, is_quoted, s, '-');  return;
    case vs_variad:       write_repr_par(f, v, is_quoted, s, '&');  return;
    case vs_seq:          write_repr_seq(f, v, is_quoted, s);       return;
  }
  assert(0);
}


// use a non-parseable flat parens to represent objects that do not have a direct repr.
#define NO_REPR_PO "⟮" // U+27EE Mathematical left flattened parenthesis.
#define NO_REPR_PC "⟯" // U+27EF Mathematical right flattened parenthesis.

static void write_repr_Func_host(CFile f, Obj func) {
  fputs(NO_REPR_PO "Func-host ", f);
  Obj d = sym_data(func.func_host->sym);
  write_data(f, d);
  fputs(NO_REPR_PC, f);
}


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Set* s) {
  // is_quoted indicates that we are writing part of a repr that has already been quoted.
  Obj_tag ot = obj_tag(o);
  if (ot == ot_int) {
    fprintf(f, "%ld", int_val(o));
  } else if (ot == ot_sym) {
    write_repr_sym(f, o, is_quoted);
  } else if (ot == ot_data) { // data-word
    // TODO: support all word values.
    assert(o.u == blank.u);
    fputs("''", f);
  } else {
    assert(ot == ot_ref);
    if (set_contains(s, o)) { // recursed
      fputs("↺", f); // anticlockwise gapped circle arrow
    } else {
      set_insert(s, o);
      assert(ot == ot_ref);
      switch (ref_tag(o)) {
        case rt_Data: write_repr_data(f, o); break;
        case rt_Vec: write_repr_vec(f, o, is_quoted, s); break;
        case rt_File: fprintf(f, NO_REPR_PO "File %p" NO_REPR_PC, file_cfile(o)); break;
        case rt_Func_host: write_repr_Func_host(f, o); break;
      }
      set_remove(s, o);
    }
  }
#if DEBUG
  err_flush();
#endif
}


static void write_repr(CFile f, Obj o) {
  Set s = set0;
  write_repr_obj(f, o, false, &s);
  set_dealloc(&s);
}
