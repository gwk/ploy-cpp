// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "23-host.h"


static void write_data(CFile f, Obj d) {
  assert(ref_is_data(d));
  Chars p = data_ptr(d);
  fwrite(p, 1, cast(Uns, data_len(d)), f);
}


static void write_repr_Sym(CFile f, Obj s, Bool is_quoted) {
  assert(obj_is_sym(s));
  if (!is_quoted && !sym_is_special(s)) {
    fputc('`', f);
  }
  Obj d = sym_data(s);
  write_data(f, d);
}


static void write_repr_Data(CFile f, Obj d, Bool is_quoted, Int depth, Set* set) {
  assert(ref_is_data(d));
  Chars p = data_ptr(d);
  fputc('\'', f);
  for_in(i, data_len(d)) {
    fputs(char_repr(p[i]), f);
  }
  fputc('\'', f);
}


// use a non-parseable flat parens to represent objects that do not have a direct repr.
#define NO_REPR_PO "⟮" // U+27EE Mathematical left flattened parenthesis.
#define NO_REPR_PC "⟯" // U+27EF Mathematical right flattened parenthesis.

static void write_repr_Env(CFile f, Obj env, Bool is_quoted, Int depth, Set* set) {
  Obj top_key = env.e->key;
  fputs(NO_REPR_PO "Env ", f);
  write_repr(f, top_key);
  fputs(NO_REPR_PC, f);
}


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Int depth, Set* set);

static void write_repr_Bang(CFile f, Obj e, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(e) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('!', f);
  write_repr_obj(f, cmpd_el(e, 0), true, depth, set);
}


static void write_repr_Quo(CFile f, Obj q, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(q) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('`', f);
  write_repr_obj(f, cmpd_el(q, 0), true, depth, set);
}


static void write_repr_Qua(CFile f, Obj q, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(q) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('~', f);
  write_repr_obj(f, cmpd_el(q, 0), true, depth, set);
}


static void write_repr_Unq(CFile f, Obj q, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(q) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc(',', f);
  write_repr_obj(f, cmpd_el(q, 0), true, depth, set);
}


static void write_repr_Label(CFile f, Obj p, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(p) == 3);
  if (!is_quoted) {
    fputc('`', f);
  }
  Obj* els = cmpd_els(p);
  Obj name = els[0];
  Obj type = els[1];
  Obj expr = els[2];
  fputc('-', f);
  write_repr_obj(f, name, true, depth, set);
  if (!is(type, s_nil)) {
    fputc(':', f);
    write_repr_obj(f, type, true, depth, set);
  }
  if (!is(expr, s_void)) {
    fputc('=', f);
    write_repr_obj(f, expr, true, depth, set);
  }
}


static void write_repr_Variad(CFile f, Obj p, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(p) == 2);
  if (!is_quoted) {
    fputc('`', f);
  }
  Obj* els = cmpd_els(p);
  Obj expr = els[0];
  Obj type = els[1];
  fputc('&', f);
  write_repr_obj(f, expr, true, depth, set);
  if (!is(type, s_nil)) {
    fputc(':', f);
    write_repr_obj(f, type, true, depth, set);
  }
}


static void write_repr_syn_seq(CFile f, Obj s, Bool is_quoted, Int depth, Set* set,
  Chars_const chars_open, Char char_close) {

  assert(ref_is_cmpd(s));
  if (!is_quoted) {
    fputc('`', f);
  }
  fputs(chars_open, f);
  for_in(i, cmpd_len(s)) {
    if (i) fputc(' ', f);
    write_repr_obj(f, cmpd_el(s, i), true, depth, set);
  }
  fputc(char_close, f);
}


static void write_repr_default(CFile f, Obj c, Bool is_quoted, Int depth, Set* set) {
  assert(ref_is_cmpd(c));
  if (is_quoted) fputs("¿Q?", f);
  fputs("{:", f);
  Obj t = obj_type(c);
  assert(obj_is_type(t));
  assert(obj_is_sym(t.t->name));
  write_repr_obj(f, t.t->name, true, depth, set);
  for_in(i, cmpd_len(c)) {
    fputc(' ', f);
    write_repr_obj(f, cmpd_el(c, i), is_quoted, depth, set);
  }
  fputc('}', f);
  if (is_quoted) fputs("¿?", f);
}


static Bool write_repr_is_quotable(Obj o) {
  if (!obj_is_valid_ref(o)) return true; // Ptr and obj0 do not have true repr.
  Obj type = ref_type(o);
  if (is(type, t_Data) || is(type, t_Env)) return true; // env does not have true repr.
  assert(ref_is_cmpd(o));
  if (!(is(type, t_Bang)
    || is(type, t_Quo)
    || is(type, t_Qua)
    || is(type, t_Unq)
    || is(type, t_Label)
    || is(type, t_Variad)
    || is(type, t_Syn_struct)
    || is(type, t_Syn_struct_typed)
    || is(type, t_Syn_seq)
    || is(type, t_Syn_seq_typed)
    || is(type, t_Expand)
    || is(type, t_Call))) {
    return false; // not an Expr.
  }
  it_cmpd(it, o) {
    if (!write_repr_is_quotable(*it)) return false;
  }
  return true;
}


static void write_repr_dispatch(CFile f, Obj s, Bool is_quoted, Int depth, Set* set) {
  Obj type = obj_type(s);

  #define DISP(t) \
  if (is(type, t_##t)) { write_repr_##t(f, s, is_quoted, depth, set); return; }

  DISP(Data);
  DISP(Env);
  if (!write_repr_is_quotable(s)) {
    assert(!is_quoted);
    write_repr_default(f, s, is_quoted, depth, set);
    return;
  }
  DISP(Bang);
  DISP(Quo);
  DISP(Qua);
  DISP(Unq);
  DISP(Label);
  DISP(Variad);
  #undef DISP

  #define DISP_SEQ(t, o, c) \
  if (is(type, t_##t)) { write_repr_syn_seq(f, s, is_quoted, depth, set, o, c); return; }

  DISP_SEQ(Syn_struct, "{", '}');
  DISP_SEQ(Syn_struct_typed, "{:", '}');
  DISP_SEQ(Syn_seq, "[", ']');
  DISP_SEQ(Syn_seq_typed, "[:", ']');
  DISP_SEQ(Expand, "<", '>');
  DISP_SEQ(Call, "(", ')');
  #undef DISP_SEQ

  write_repr_default(f, s, is_quoted, depth, set);
}


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  // is_quoted indicates that we are writing part of a repr that has already been quoted.
  Obj_tag ot = obj_tag(o);
  if (ot == ot_ptr) {
#if OPTION_BLANK_PTR_REPR
    fprintf(f, NO_REPR_PO "Ptr" NO_REPR_PC);
#else
    fprintf(f, NO_REPR_PO "%p" NO_REPR_PC, ptr_val(o));
#endif
  } else if (ot == ot_int) {
    fprintf(f, "%ld", int_val(o));
  } else if (ot == ot_sym) {
    if (obj_is_data_word(o)) {
      // TODO: support all word values.
      assert(is(o, blank));
      fputs("''", f);
    } else {
      write_repr_Sym(f, o, is_quoted);
    }
  } else {
    assert(ot == ot_ref);
    if (is(o, obj0)) {
      fputs(NO_REPR_PO "obj0" NO_REPR_PC, f);
      return;
    }
    if (depth > 8) {
      fputs("…", f); // ellipsis.
    } else if (set_contains(set, o)) { // cyclic object recursed.
      fputs("↺", f); // anticlockwise gapped circle arrow.
    } else {
      set_insert(set, o);
      write_repr_dispatch(f, o, is_quoted, depth + 1, set);
      set_remove(set, o);
    }
  }
#if !OPT
  err_flush();
#endif
}


static void write_repr(CFile f, Obj o) {
  Set s = set0;
  write_repr_obj(f, o, false, 0, &s);
  set_dealloc(&s);
}
