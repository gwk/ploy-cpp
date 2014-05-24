/// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "19-write-repr.h"


typedef struct {
  Int pos;
  Int line;
  Int col;
} Src_pos;

// main parser object holds the input string, parse state, and source location info.
typedef struct {
  Str  src;  // input string.
  Str  path; // input path for error reporting.
  Src_pos sp;
  Chars e; // error message.
} Parser;


// character terminates a syntactic sequence.
static Bool char_is_seq_term(Char c) {
  return
  c == ')' ||
  c == ']' ||
  c == '}' ||
  c == '>';
}


// character terminates an atom.
static Bool char_is_atom_term(Char c) {
  return c == '|' || isspace(c) || char_is_seq_term(c);
}


static void parse_err(Parser* p) {
  errF("%.*s:%ld:%ld (%ld): ", FMT_STR(p->path), p->sp.line + 1, p->sp.col + 1, p->sp.pos);
  if (p->e) errF("\nerror: %s\nobj:  ", p->e);
}


UNUSED_FN static void parse_errL(Parser* p) {
  parse_err(p);
  err_nl();
}


 __attribute__((format (printf, 2, 3)))
static Obj parse_error(Parser* p, CharsC fmt, ...) {
  assert(!p->e);
  va_list args;
  va_start(args, fmt);
  Chars msg;
  Int msg_len = vasprintf(&msg, fmt, args);
  va_end(args);
  check(msg_len >= 0, "parse_error allocation failed: %s", fmt);
  // parser owner must call raw_dealloc on e.
  p->e = str_src_loc_str(p->src, p->path, p->sp.pos, 0, p->sp.line, p->sp.col, msg);
  free(msg); // matches vasprintf.
  return obj0;
}


#define parse_check(condition, fmt, ...) \
if (!(condition)) return parse_error(p, fmt, ##__VA_ARGS__)


#define PP  (p->sp.pos < p->src.len)
#define PP1 (p->sp.pos < p->src.len - 1)

#define PC  p->src.chars[p->sp.pos]
#define PC1 p->src.chars[p->sp.pos + 1]

#define P_ADV(n) { p->sp.pos += n; p->sp.col += n; }
#define P_ADV1 P_ADV(1)


static U64 parse_U64(Parser* p) {
  Char c = PC;
  I32 base = 0;
  if (c == '0' && PP1) {
    Char c1 = PC1;
    switch (c1) {
      case 'b': base = 2;   break;
      case 'q': base = 4;   break;
      case 'o': base = 8;   break;
      case 'd': base = 10;  break;
      case 'h': base = 16;  break;
    }
    if (base) {
      P_ADV(2);
    }
    else {
      base = 10;
    }
  }
  Chars start = p->src.chars + p->sp.pos;
  Chars end;
  U64 u = strtoull(start, &end, base);
  int en = errno;
  if (en) {
    parse_error(p, "malformed number literal: %s", strerror(en));
    return 0;
  }
  assert(end > start); // strtoull man page is a bit confusing as to the precise semantics.
  Int n = end - start;
  P_ADV(n);
  if (PP && !char_is_atom_term(PC)) {
    parse_error(p, "invalid number literal terminator: %c", PC);
    return 0;
  }
  return u;
}


static Obj parse_uns(Parser* p) {
  return new_uns(parse_U64(p));
}


static Obj parse_int(Parser* p, Int sign) {
  assert(PC == '-' || PC == '+');
  P_ADV1;
  U64 u = parse_U64(p);
  parse_check(u <= max_Int, "signed number literal is too large");
  return new_int((I64)u * sign);
}


static Obj parse_sym(Parser* p) {
  assert(PC == '_' || isalpha(PC));
  Int pos = p->sp.pos;
  loop {
    P_ADV1;
    if (!PP) break;
    Char c = PC;
    if (!(c == '-' || c == '_' || isalnum(c))) break;
  }
  Str s = str_slice(p->src, pos, p->sp.pos);
  return new_sym(s);
}


static Obj parse_expr(Parser* p);


static Obj parse_comment(Parser* p) {
  assert(PC == '#');
  if (PP1 && PC1 == '#') { // double-hash comments out the following expression.
    P_ADV(2);
    Obj expr = parse_expr(p);
    if (p->e) return obj0;
    // the QUO prevents macro expansion of the commented code,
    // and also differentiates it from line comments.
    return new_vec2(obj_ret_val(COMMENT), new_vec2(obj_ret_val(QUO), expr));
  }
  // otherwise comment out a single line.
  Src_pos sp_report = p->sp;
  do {
    P_ADV1;
    if (!PP) {
      p->sp = sp_report; // better error message.
      return parse_error(p, "unterminated comment (add newline)");
    }
  }  while (PC == ' ');
  Int pos_start = p->sp.pos;
  do {
    P_ADV1;
    if (!PP) {
      p->sp = sp_report; // better error message.
      return parse_error(p, "unterminated comment (add newline)");
    }
  }  while (PC != '\n');
  Str s = str_slice(p->src, pos_start, p->sp.pos);
  Obj d = new_data_from_str(s);
  return new_vec2(obj_ret_val(COMMENT), d);
}


static Obj parse_data(Parser* p, Char q) {
  assert(PC == q);
  Src_pos sp_open = p->sp; // for error reporting.
  Str s = str_alloc(size_min_alloc);
  Int i = 0;

#define APPEND(c) { i = str_append(&s, i, c); }

  Bool escape = false;
  loop {
    P_ADV1;
    if (!PP) {
      p->sp = sp_open; // better error message.
      return parse_error(p, "unterminated string literal");
    }
    Char c = PC;
    if (escape) {
      escape = false;
      Char ce = c;
      switch (c) {
        case '0': ce = 0;     break; // NULL
        case 'a': ce = '\a';  break; // bell - BEL
        case 'b': ce = '\b';  break; // backspace - BS
        case 'f': ce = '\f';  break; // form feed - FF
        case 'n': ce = '\n';  break; // line feed - LF
        case 'r': ce = '\r';  break; // carriage return - CR
        case 't': ce = '\t';  break; // horizontal tab - TAB
        case 'v': ce = '\v';  break; // vertical tab - VT
        case '\\':  break;
        case '\'':  break;
        case '"':   break;
        case 'x': // ordinal escapes not yet implemented. \xx byte value.
        case 'u': // \uuuu unicode.
        case 'U': // \UUUUUU unicode. 6 or 8 chars? utf-8 is restricted to end at U+10FFFF.
        default: APPEND('\\'); // not a valid escape code.
      }
      APPEND(ce);
    }
    else {
      if (c == q) break;
      if (c == '\\') escape = true;
      else APPEND(c);
    }
  }
  #undef APPEND
  P_ADV1; // past closing quote.
  Obj d = new_data_from_str(str_mk(i, s.chars));
  str_dealloc(s);
  return d;
}


static Bool parse_space(Parser* p) {
  while (PP) {
    Char c = PC;
    //parse_err(p); errFL("space? '%c'", c);
    switch (c) {
      case ' ':
        P_ADV1;
        continue;
      case '\n':
        p->sp.pos++;
        p->sp.line++;
        p->sp.col = 0;
        continue;
      case '\t':
        parse_error(p, "illegal tab character");
        return false;
      default:
        if (isspace(c)) {
          parse_error(p, "illegal space character: '%c' (%02x)", c, c);
          return false;
        }
        return true;
    }
  }
  return false; // EOS.
}


static Bool parse_terminator(Parser* p, Char t) {
  if (PC != t) {
    parse_error(p, "expected terminator: '%c'", t);
    return false;
  }
  P_ADV1;
  return true;
}

static Bool parser_has_next_expr(Parser* p) {
  return parse_space(p) && !char_is_seq_term(PC);
}


static Mem parse_exprs(Parser* p, Char term) {
  // caller must call mem_release_dealloc or mem_dealloc on returned Mem.
  Array a = array0;
  while (parser_has_next_expr(p)) {
    if (term && PC == term) break;
    Obj o = parse_expr(p);
    if (p->e) {
      assert(o.u == obj0.u);
      break;
    }
    array_append(&a, o);
  }
  if (p->e) {
    mem_release_dealloc(a.mem);
    return mem0;
  }
  return a.mem;
}


#define P_CONSUME_TERMINATOR(t) \
if (p->e || !parse_terminator(p, t)) { \
  mem_release_dealloc(m); \
  return obj0; \
}


static Obj parse_struct(Parser* p) {
  P_ADV1;
  Mem m = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR('}');
  Obj v = new_vec_M(m);
  mem_dealloc(m);
  return v;
}


static Obj parse_call(Parser* p) {
  P_ADV1;
  Mem m = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR(')');
  Obj v = new_vec_EM(obj_ret_val(CALL), m);
  mem_dealloc(m);
  return v;
}


static Obj parse_expand(Parser* p) {
  P_ADV1;
  Mem m = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR('>');
  Obj v = new_vec_EM(obj_ret_val(EXPAND), m);
  mem_dealloc(m);
  return v;
}


static Obj parse_seq_simple(Parser* p) {
  Mem m = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR(']');
  if (!m.len) {
    return obj_ret_val(VEC0);
  }
  Obj v = new_vec_EM(obj_ret_val(SEQ), m);
  mem_dealloc(m);
  return v;
}


static Obj parse_chain_simple(Parser* p) {
  P_ADV1;
  Mem m = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR(']');
  if (!m.len) {
    return obj_ret_val(CHAIN0);
  }
  Obj c = obj_ret_val(END);
  for_in_rev(i, m.len) {
    Obj el = mem_el_move(m, i);
    c = new_vec3(obj_ret_val(SEQ), el, c);
  }
  mem_dealloc(m);
  return c;
}


static Obj parse_chain_blocks(Parser* p) {
  Array a = array0;
  while (parser_has_next_expr(p)) {
    if (PC != '|') {
      parse_error(p, "expected '|'");
    }
    P_ADV1;
    Mem m = parse_exprs(p, '|');
    if (p->e) {
      mem_release_dealloc(a.mem);
      return obj0;
    }
    Obj v = new_vec_raw(m.len + 2);
    Obj* els = vec_els(v);
    els[0] = obj_ret_val(SEQ);
    for_in(i, m.len) {
      els[i + 1] = mem_el_move(m, i);
    }
    // temporarily fill tl with ILLEGAL; replaced by vec_put below.
    // since vec_put releases, we must retain here.
    els[m.len + 1] = obj_ret_val(ILLEGAL);
    mem_dealloc(m);
    array_append(&a, v);
  }
  Mem m = a.mem;
  P_CONSUME_TERMINATOR(']');
  // assemble the chain.
  if (!m.len) {
    return obj_ret_val(CHAIN0);
  }
  Obj c = obj_ret_val(END);
  for_in_rev(i, m.len) {
    Obj v = mem_el_move(m, i);
    vec_put(v, vec_len(v) - 1, c);
    c = v;
  }
  mem_dealloc(m);
  return c;
}


static Obj parse_seq(Parser* p) {
  P_ADV1;
  parse_space(p);
  if (PC == ':') {
    return parse_chain_simple(p);
  }
  if (PC == '|') {
    return parse_chain_blocks(p);
  }
  else {
    return parse_seq_simple(p);
  }
}


static Obj parse_qua(Parser* p) {
  assert(PC == '`');
  P_ADV1;
  Obj o = parse_expr(p);
  return new_vec2(obj_ret_val(QUA), o);
}


static Obj parse_unq(Parser* p) {
  assert(PC == ',');
  P_ADV1;
  Obj o = parse_expr(p);
  return new_vec2(obj_ret_val(UNQ), o);
}



static Obj parse_par(Parser* p, Obj sym, CharsC par_desc) {
  P_ADV1;
  Src_pos sp_open = p->sp; // for error reporting.
  Obj name = parse_expr(p);
  if (p->e) return obj0;
  if (!obj_is_symbol(name)) {
    obj_rel(name);
    p->sp = sp_open;
    return parse_error(p, "%s name is not a symbol", par_desc);
  }
  Char c = PC;
  Obj type;
  if (c == ':') {
    P_ADV1;
    type = parse_expr(p);
    c = PC;
  }
  else type = obj_ret_val(NIL);
  Obj expr;
  if (PC == '=') {
    P_ADV1;
    expr = parse_expr(p);
  }
  else expr = obj_ret_val(NIL);
  return new_vec4(obj_ret_val(sym), name, type, expr);
}


// parse an expression.
static Obj parse_expr_sub(Parser* p) {
  Char c = PC;
  switch (c) {
    case '{':   return parse_struct(p);
    case '(':   return parse_call(p);
    case '<':   return parse_expand(p);
    case '[':   return parse_seq(p);
    case '`':   return parse_qua(p);
    case ',':   return parse_unq(p);
    case '\'':  return parse_data(p, '\'');
    case '"':   return parse_data(p, '"');
    case '#':   return parse_comment(p);
    case '&':   return parse_par(p, VARIAD, "variad");
    case '+':
      if (isdigit(PC1)) return parse_int(p, 1);
      break;
    case '-':
      if (isdigit(PC1)) return parse_int(p, -1);
      return parse_par(p, LABEL, "label");

  }
  if (isdigit(c)) {
    return parse_uns(p);
  }
  if (c == '_' || isalpha(c)) {
    return parse_sym(p);
  }
  return parse_error(p, "unexpected character");
}


static Obj parse_expr(Parser* p) {
  Obj o = parse_expr_sub(p);
#if VERBOSE_PARSE
  parse_err(p); obj_errL(o);
#endif
  return o;
}


static Obj parse_src(Str path, Str src, Chars* e) {
  // caller must free e.
  Parser p = (Parser) {
    .path=path,
    .src=src,
    .sp=(Src_pos){.pos=0, .line=0, .col=0,},
    .e=NULL,
  };
  Mem m = parse_exprs(&p, 0);
  Obj o;
  if (p.e) {
    assert(m.len == 0 && m.els == NULL);
    o = obj0;
  }
  else if (p.sp.pos != p.src.len) {
    o = parse_error(&p, "parsing terminated early");
    mem_release_dealloc(m);
  }
  else {
    o = new_vec_M(m);
    mem_dealloc(m);
  }
  *e = p.e;
  return o;
}

