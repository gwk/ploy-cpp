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


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Int depth, Set* set);

static void write_repr_struct_struct(CFile f, Obj s, Bool is_quoted, Int depth, Set* set) {
  assert(ref_is_struct(s));
  if (is_quoted) fputs("???", f);
  fputc('[', f);
  Mem m = struct_mem(s);
  for_in(i, m.len) {
    if (i) fputc(' ', f);
    write_repr_obj(f, m.els[i], is_quoted, depth, set);
  }
  fputc(']', f);
}


static void write_repr_quo(CFile f, Obj q, Bool is_quoted, Int depth, Set* set) {
  assert(struct_len(q) == 2);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('`', f);
  write_repr_obj(f, struct_el(q, 1), true, depth, set);
}


static void write_repr_qua(CFile f, Obj q, Bool is_quoted, Int depth, Set* set) {
  assert(struct_len(q) == 2);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('~', f);
  write_repr_obj(f, struct_el(q, 1), true, depth, set);
}


static void write_repr_unq(CFile f, Obj q, Bool is_quoted, Int depth, Set* set) {
  assert(struct_len(q) == 2);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc(',', f);
  write_repr_obj(f, struct_el(q, 1), true, depth, set);
}


static void write_repr_par(CFile f, Obj p, Bool is_quoted, Int depth, Set* set, Char c) {
  assert(struct_len(p) == 4);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc(c, f);
  Obj* els = struct_els(p);
  Obj name = els[1];
  Obj type = els[2];
  Obj expr = els[3];
  write_repr_obj(f, name, true, depth, set);
  if (type.u != s_nil.u) {
    fputc(':', f);
    write_repr_obj(f, type, true, depth, set);
  }
  if (expr.u != s_nil.u) {
    fputc('=', f);
    write_repr_obj(f, expr, true, depth, set);
  }
}


static void write_repr_seq(CFile f, Obj s, Bool is_quoted, Int depth, Set* set) {
  assert(ref_is_struct(s));
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('[', f);
  Mem m = struct_mem(s);
  for_imn(i, 1, m.len) {
    if (i > 1) fputc(' ', f);
    write_repr_obj(f, m.els[i], true, depth, set);
  }
  fputc(']', f);
}


static void write_repr_struct(CFile f, Obj s, Bool is_quoted, Int depth, Set* set) {
  assert(ref_is_struct(s));
  switch (struct_shape(s)) {
    case ss_struct: write_repr_struct_struct(f, s, is_quoted, depth, set); return;
    case ss_quo:    write_repr_quo(f, s, is_quoted, depth, set);      return;
    case ss_qua:    write_repr_qua(f, s, is_quoted, depth, set);      return;
    case ss_unq:    write_repr_unq(f, s, is_quoted, depth, set);      return;
    case ss_label:  write_repr_par(f, s, is_quoted, depth, set, '-'); return;
    case ss_variad: write_repr_par(f, s, is_quoted, depth, set, '&'); return;
    case ss_seq:    write_repr_seq(f, s, is_quoted, depth, set);      return;
  }
  assert(0);
}


// use a non-parseable flat parens to represent objects that do not have a direct repr.
#define NO_REPR_PO "⟮" // U+27EE Mathematical left flattened parenthesis.
#define NO_REPR_PC "⟯" // U+27EF Mathematical right flattened parenthesis.



static void write_repr_env(CFile f, Obj env) {
  // TODO: show frame and binding count.
  fprintf(f, NO_REPR_PO "Env %p" NO_REPR_PC, env.r);
}


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  // is_quoted indicates that we are writing part of a repr that has already been quoted.
  Obj_tag ot = obj_tag(o);
  if (ot == ot_ptr) {
    fprintf(f, "%p", ptr_val(o));
  } else if (ot == ot_int) {
    fprintf(f, "%ld", int_val(o));
  } else if (ot == ot_sym) {
    if (obj_is_data_word(o)) {
      // TODO: support all word values.
      assert(o.u == blank.u);
      fputs("''", f);
    } else {
      write_repr_sym(f, o, is_quoted);
    }
  } else {
    assert(ot == ot_ref);
    depth++;
    if (depth > 8) {
      fputs("…", f);
    } else if (set_contains(set, o)) { // recursed
      fputs("↺", f); // anticlockwise gapped circle arrow
    } else {
      set_insert(set, o);
      assert(ot == ot_ref);
      switch (ref_tag(o)) {
        case rt_Data: write_repr_data(f, o); break;
        case rt_Struct: write_repr_struct(f, o, is_quoted, depth, set); break;
        case rt_Env: write_repr_env(f, o); break;
        default: assert(0);
      }
      set_remove(set, o);
    }
  }
#if DEBUG
  err_flush();
#endif
}


static void write_repr(CFile f, Obj o) {
  Set s = set0;
  write_repr_obj(f, o, false, 0, &s);
  set_dealloc(&s);
}
