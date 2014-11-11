// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "24-host.h"


static void write_data(CFile f, Obj d) {
  assert(ref_is_data(d));
  fwrite(data_chars(d), 1, cast(Uns, data_len(d)), f);
}


static void write_repr_Sym(CFile f, Obj s, Bool is_quoted) {
  assert(s.is_sym());
  if (!is_quoted && !sym_is_special(s)) {
    fputc('`', f);
  }
  Obj d = sym_data(s);
  write_data(f, d);
}


static void write_repr_Data(CFile f, Obj d) {
  assert(ref_is_data(d));
  Chars p = data_ref_chars(d);
  fputc('\'', f);
  for_in(i, data_len(d)) {
    fputs(char_repr(p[i]), f);
  }
  fputc('\'', f);
}


// use a non-parseable flat parens to represent objects that do not have a direct repr.
#define NO_REPR_PO "⟮" // U+27EE Mathematical left flattened parenthesis.
#define NO_REPR_PC "⟯" // U+27EF Mathematical right flattened parenthesis.


static void write_repr_Env(CFile f, Obj env) {
  Obj top_key = env.e->key;
  fputs(NO_REPR_PO "Env ", f);
  write_repr_Sym(f, top_key, true);
  fputs(NO_REPR_PC, f);
}


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Int depth, Set* set);

static void write_repr_Comment(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 2);
  fputs(NO_REPR_PO "#", f);
  if (bool_is_true(cmpd_el(o, 0))) {
    fputs("#", f);
  } else {
    fputs(" ", f);
  }
  write_repr_obj(f, cmpd_el(o, 1), false, depth, set);
  fputs(NO_REPR_PC, f);
}


static void write_repr_Bang(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('!', f);
  write_repr_obj(f, cmpd_el(o, 0), true, depth, set);
}


static void write_repr_Quo(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('`', f);
  write_repr_obj(f, cmpd_el(o, 0), true, depth, set);
}


static void write_repr_Qua(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('~', f);
  write_repr_obj(f, cmpd_el(o, 0), true, depth, set);
}


static void write_repr_Unq(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc(',', f);
  write_repr_obj(f, cmpd_el(o, 0), true, depth, set);
}


static void write_repr_Splice(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('*', f);
  write_repr_obj(f, cmpd_el(o, 0), true, depth, set);
}


static void write_repr_Label(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 3);
  if (!is_quoted) {
    fputc('`', f);
  }
  Obj* els = cmpd_els(o);
  Obj name = els[0];
  Obj type = els[1];
  Obj expr = els[2];
  fputc('-', f);
  write_repr_obj(f, name, true, depth, set);
  if (type != s_nil) {
    fputc(':', f);
    write_repr_obj(f, type, true, depth, set);
  }
  if (expr != s_void) {
    fputc('=', f);
    write_repr_obj(f, expr, true, depth, set);
  }
}


static void write_repr_Variad(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 2);
  if (!is_quoted) {
    fputc('`', f);
  }
  Obj* els = cmpd_els(o);
  Obj expr = els[0];
  Obj type = els[1];
  fputc('&', f);
  write_repr_obj(f, expr, true, depth, set);
  if (type != s_nil) {
    fputc(':', f);
    write_repr_obj(f, type, true, depth, set);
  }
}


static void write_repr_Accessor(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('.', f);
  write_repr_obj(f, cmpd_el(o, 0), true, depth, set);
}


static void write_repr_Mutator(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  assert(cmpd_len(o) == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputs(".=", f);
  write_repr_obj(f, cmpd_el(o, 0), true, depth, set);
}


static void write_repr_syn_seq(CFile f, Obj s, Bool is_quoted, Int depth, Set* set,
  Chars chars_open, Char char_close) {
  assert(cmpd_len(s) == 1);
  Obj exprs = cmpd_el(s, 0);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputs(chars_open, f);
  for_in(i, cmpd_len(exprs)) {
    if (i) fputc(' ', f);
    write_repr_obj(f, cmpd_el(exprs, i), true, depth, set);
  }
  fputc(char_close, f);
}


static void write_repr_default(CFile f, Obj c, Bool is_quoted, Int depth, Set* set) {
  assert(ref_is_cmpd(c));
  if (is_quoted) fputs("¿", f);
  fputs("(", f);
  Obj t = obj_type(c);
  assert(obj_is_type(t));
  assert(t.t->name.is_sym());
  write_repr_obj(f, t.t->name, true, depth, set);
  for_in(i, cmpd_len(c)) {
    fputc(' ', f);
    write_repr_obj(f, cmpd_el(c, i), false, depth, set);
  }
  fputc(')', f);
}


static void write_repr_dispatch(CFile f, Obj s, Bool is_quoted, Int depth, Set* set) {
  Obj type = obj_type(s);
  if (type == t_Data) { write_repr_Data(f, s); return; }
  if (type == t_Env)  { write_repr_Env(f, s); return; }

  #define DISP(t) \
  if (type == t_##t) { write_repr_##t(f, s, is_quoted, depth, set); return; }
  DISP(Comment);
  DISP(Bang);
  DISP(Quo);
  DISP(Qua);
  DISP(Unq);
  DISP(Splice);
  DISP(Label);
  DISP(Variad);
  DISP(Accessor);
  DISP(Mutator);
  #undef DISP

  #define DISP_SEQ(t, o, c) \
  if (type == t_##t) { write_repr_syn_seq(f, s, is_quoted, depth, set, o, c); return; }

  DISP_SEQ(Syn_struct, "{", '}');
  DISP_SEQ(Syn_seq, "[", ']');
  DISP_SEQ(Expand, "<", '>');
  DISP_SEQ(Call, "(", ')');
  #undef DISP_SEQ

  write_repr_default(f, s, is_quoted, depth, set);
}


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Int depth, Set* set) {
  // is_quoted indicates that we are writing part of a repr that has already been quoted.
  if (!o.vld()) {
    fputs(NO_REPR_PO "obj0" NO_REPR_PC, f);
    return;
  }
  Obj_tag ot = o.tag();
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
      assert(o == blank);
      fputs("''", f);
    } else {
      write_repr_Sym(f, o, is_quoted);
    }
  } else {
    assert(ot == ot_ref);
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

