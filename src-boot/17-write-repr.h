// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "16-type.h"


static void write_data(CFile f, Obj d) {
  assert(d.ref_is_data());
  fwrite(d.data_chars(), 1, Uns(d.data_len()), f);
}


static void write_repr_Sym(CFile f, Obj s, Bool is_quoted) {
  assert(s.is_sym());
  if (!is_quoted && !s.is_special_sym()) {
    fputc('`', f);
  }
  Obj d = s.sym_data();
  write_data(f, d);
}


static void write_repr_Data(CFile f, Obj d) {
  assert(d.ref_is_data());
  Chars p = d.data_ref_chars();
  fputc('\'', f);
  for_in(i, d.data_len()) {
    fputs(char_repr(p[i]), f);
  }
  fputc('\'', f);
}


#if 0
// use a non-parseable flat parens to represent objects that do not have a direct repr.
#define NO_REPR_PO "⟮" // U+27EE Mathematical left flattened parenthesis.
#define NO_REPR_PC "⟯" // U+27EF Mathematical right flattened parenthesis.
#else
#define NO_REPR_PO "("
#define NO_REPR_PC ")"
#endif


static void write_repr_Env(CFile f, Obj env) {
  Obj top_key = env.e->key;
  fputs(NO_REPR_PO "Env ", f);
  write_repr_Sym(f, top_key, true);
  fputs(NO_REPR_PC, f);
}


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Int depth, Set& set);

static void write_repr_Comment(CFile f, Obj o, UNUSED Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 2);
  fputs(NO_REPR_PO "#", f);
  if (o.cmpd_el(0).is_true_bool()) {
    fputs("#", f);
  } else {
    fputs(" ", f);
  }
  write_repr_obj(f, o.cmpd_el(1), false, depth, set);
  fputs(NO_REPR_PC, f);
}


static void write_repr_Bang(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('!', f);
  write_repr_obj(f, o.cmpd_el(0), true, depth, set);
}


static void write_repr_Quo(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('`', f);
  write_repr_obj(f, o.cmpd_el(0), true, depth, set);
}


static void write_repr_Qua(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('~', f);
  write_repr_obj(f, o.cmpd_el(0), true, depth, set);
}


static void write_repr_Unq(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc(',', f);
  write_repr_obj(f, o.cmpd_el(0), true, depth, set);
}


static void write_repr_Splice(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('*', f);
  write_repr_obj(f, o.cmpd_el(0), true, depth, set);
}


static void write_repr_Label(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 3);
  if (!is_quoted) {
    fputc('`', f);
  }
  Obj* els = o.cmpd_els();
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


static void write_repr_Variad(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 2);
  if (!is_quoted) {
    fputc('`', f);
  }
  Obj* els = o.cmpd_els();
  Obj expr = els[0];
  Obj type = els[1];
  fputc('&', f);
  write_repr_obj(f, expr, true, depth, set);
  if (type != s_nil) {
    fputc(':', f);
    write_repr_obj(f, type, true, depth, set);
  }
}


static void write_repr_Accessor(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputc('.', f);
  write_repr_obj(f, o.cmpd_el(0), true, depth, set);
}


static void write_repr_Mutator(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
  assert(o.cmpd_len() == 1);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputs(".=", f);
  write_repr_obj(f, o.cmpd_el(0), true, depth, set);
}


static void write_repr_syn_seq(CFile f, Obj s, Bool is_quoted, Int depth, Set& set,
  Chars chars_open, Char char_close) {
  assert(s.cmpd_len() == 1);
  Obj exprs = s.cmpd_el(0);
  if (!is_quoted) {
    fputc('`', f);
  }
  fputs(chars_open, f);
  for_in(i, exprs.cmpd_len()) {
    if (i) fputc(' ', f);
    write_repr_obj(f, exprs.cmpd_el(i), true, depth, set);
  }
  fputc(char_close, f);
}


static void write_repr_default(CFile f, Obj c, Bool is_quoted, Int depth, Set& set) {
  assert(c.ref_is_cmpd());
  if (is_quoted) fputs("¿", f);
  fputs("(", f);
  Obj t = c.type();
  assert(t.is_type());
  write_repr_obj(f, t.t->name, true, depth, set);
  for_in(i, c.cmpd_len()) {
    fputc(' ', f);
    write_repr_obj(f, c.cmpd_el(i), false, depth, set);
  }
  fputc(')', f);
}


static void write_repr_dispatch(CFile f, Obj s, Bool is_quoted, Int depth, Set& set) {
  Obj type = s.type();
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


static void write_repr_obj(CFile f, Obj o, Bool is_quoted, Int depth, Set& set) {
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
    fprintf(f, NO_REPR_PO "%p" NO_REPR_PC, o.ptr());
#endif
  } else if (ot == ot_int) {
    fprintf(f, "%ld", o.int_val());
  } else if (ot == ot_sym) {
    if (o.is_data_word()) {
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
    } else if (set.contains(o)) { // cyclic object recursed.
      fputs("↺", f); // anticlockwise gapped circle arrow.
    } else {
      set.insert(o);
      write_repr_dispatch(f, o, is_quoted, depth + 1, set);
      set.remove(o);
    }
  }
#if !OPT
  err_flush();
#endif
}


static void write_repr(CFile f, Obj o) {
  Set s;
  write_repr_obj(f, o, false, 0, s);
  assert(!s.len());
  s.dealloc(true);
}
