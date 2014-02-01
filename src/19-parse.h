// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-write-repr.h"


typedef struct {
  Int pos;
  Int line;
  Int col;
} Src_pos;

// main parser object holds the input string, parse state, and source location info.
typedef struct {
  SS  src;  // input string.
  SS  path; // input path for error reporting.
  Src_pos sp;
  BC e; // error message.
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
  errF("%*s:%ld:%ld (%ld): ", FMT_SS(p->path), p->sp.line + 1, p->sp.col + 1, p->sp.pos);
  if (p->e) errF("\nerror: %s\nobj:  ", p->e);
}


static void parse_errL(Parser* p) {
  parse_err(p);
  err_nl();
}


 __attribute__((format (printf, 2, 3)))
static Obj parse_error(Parser* p, BC fmt, ...) {
  assert(!p->e);
  va_list args;
  va_start(args, fmt);
  BM msg;
  Int msg_len = vasprintf(&msg, fmt, args);
  va_end(args);
  check(msg_len >= 0, "parse_error allocation failed: %s", fmt);
  // e must be freed by parser owner.
  p->e = ss_src_loc_str(p->src, p->path, p->sp.pos, 0, p->sp.line, p->sp.col, msg);
  raw_dealloc(msg);
  return ILLEGAL;
}


#define parse_check(condition, fmt, ...) \
if (!(condition)) return parse_error(p, fmt, ##__VA_ARGS__)


#define PP  (p->sp.pos < p->src.len)
#define PP1 (p->sp.pos < p->src.len - 1)
#define PP2 (p->sp.pos < p->src.len - 2)

#define PC  p->src.b.c[p->sp.pos]
#define PC1 p->src.b.c[p->sp.pos + 1]
#define PC2 p->src.b.c[p->sp.pos + 2]

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
  BC start = p->src.b.c + p->sp.pos;
  BM end;
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
  SS s = ss_slice(p->src, pos, p->sp.pos);
  return new_sym(s);
}


static Obj parse_comment(Parser* p) {
  assert(PC == '#');
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
  SS s = ss_slice(p->src, pos_start, p->sp.pos);
  Obj d = new_data_from_SS(s);
  return new_vec2(obj_ret_val(COMMENT), d);
}


static Obj parse_data(Parser* p, Char q) {
  assert(PC == q);
  Src_pos sp_open = p->sp; // for error reporting.
  SS s = ss_alloc(16); // could declare as static to reduce reallocs.
  Int i = 0;

#define APPEND(c) { \
  if (i == s.len) ss_realloc(&s, round_up_to_power_2(s.len + 3)); \
  assert(i < s.len); \
  s.b.m[i++] = c; \
}

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
  Obj d = new_data_from_SS(ss_mk(i, s.b));
  ss_dealloc(s);
  return d;
}


static Obj parse_expr(Parser* p);


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


static Mem parse_seq(Parser* p, Char term) {
  // caller must free return Mem.
  Array a = array0;
  while (parser_has_next_expr(p)) {
    if (term && PC == term) break;
    Obj o = parse_expr(p);
    if (p->e) {
      mem_release_dealloc(a.mem);
      return mem0;
    }
    array_append_move(&a, o);
  }
  return a.mem;
}


static Mem parse_blocks(Parser* p) {
  // caller must free return Mem.
  Array a = array0;
  while (parser_has_next_expr(p)) {
    if (PC != '|') {
      parse_error(p, "expected '|'");
    }
    P_ADV1;
    Mem m = parse_seq(p, '|');
    if (p->e) {
      mem_release_dealloc(a.mem);
      return mem0;
    }
    // alloc extra element for chaining; temporarily set to ILLEGAL, which is retain-counted to match the vec_put call.
    Obj o = new_vec_HM(obj_ret_val(ILLEGAL), m);
    mem_dealloc(m);
    array_append_move(&a, o);
  }
  //for_in(i, a.mem.len) { errF("  pb %ld: ", i); dbg(a.mem.els[i]); }
  return a.mem;
}


#define P_ADV_TERM(t) \
if (p->e || !parse_terminator(p, t)) { \
  mem_release_dealloc(m); \
  return ILLEGAL; \
}


static Obj parse_call(Parser* p) {
  P_ADV1;
  Mem m = parse_seq(p, 0);
  P_ADV_TERM(')');
  Obj v = new_vec_HM(obj_ret_val(CALL), m);
  mem_dealloc(m);
  return v;
}


static Obj parse_expa(Parser* p) {
  P_ADV1;
  Mem m = parse_seq(p, 0);
  P_ADV_TERM('>');
  Obj v = new_vec_HM(obj_ret_val(EXPA), m);
  mem_dealloc(m);
  return v;
}


static Obj parse_vec(Parser* p) {
  P_ADV1;
  Mem m = parse_seq(p, 0);
  P_ADV_TERM(']');
  Obj v = new_vec_M(m);
  mem_dealloc(m);
  Obj q = new_vec2(obj_ret_val(QUO), v);
  return q;
}


static Obj parse_chain(Parser* p) {
  P_ADV1;
  Mem m;
  Obj c;
  parse_space(p);
  if (PC == '|') {
    m = parse_blocks(p);
    c =  new_chain_blocks_M(m);
  }
  else {
    m = parse_seq(p, 0);
    c = new_chain_M(m);
  }
  mem_dealloc(m);
  P_ADV_TERM('}');
  Obj q = new_vec2(obj_ret_val(QUO), c);
  return q;
}


static Obj parse_qua(Parser* p) {
  assert(PC == '`');
  P_ADV1;
  Obj o = parse_expr(p);
  return new_vec2(obj_ret_val(QUA), o);
}


static Obj parse_dequote(Parser* p) {
  assert(PC == '~');
  P_ADV1;
  Src_pos sp_sub = p->sp;
  Obj o = parse_expr(p);
  if (p->e) return ILLEGAL;
  if (!(obj_is_vec(o) && ref_len(o) == 2 && vec_el(o, 0).u == QUO.u)) {
    p->sp = sp_sub; // better error message.
    parse_error(p, "dequote expected quoted subexpression");
  }
  Obj sub = vec_el(o, 1);
  obj_ret(sub);
  obj_rel(o);
  return sub;
}


static Obj parse_label_sub(Parser* p, Obj sym) {
  P_ADV1;
  Obj name = parse_expr(p);
  if (p->e) return ILLEGAL;
  Char c = PC;
  Obj type = NIL;
  if (c == ':') {
    P_ADV1;
    type = parse_expr(p);
    c = PC;
  }
  Obj val = NIL;
  if (PC == '=') {
    P_ADV1;
    val = parse_expr(p);
  }
  return new_vec4(obj_ret_val(sym), name, type, val);
}


static Obj parse_label(Parser* p) {
  return parse_label_sub(p, LABEL);
}


static Obj parse_variad(Parser* p) {
  return parse_label_sub(p, VARIAD);
}


// parse an expression.
static Obj parse_expr_sub(Parser* p) {
  Char c = PC;
  switch (c) {
    case '(':   return parse_call(p);
    case '<':   return parse_expa(p);
    case '[':   return parse_vec(p);
    case '{':   return parse_chain(p);
    case '`':   return parse_qua(p);
    case '\'':  return parse_data(p, '\'');
    case '"':   return parse_data(p, '"');
    case '#':   return parse_comment(p);
    case '~':   return parse_dequote(p);
    case '&':   return parse_variad(p);
    case '+':
      if (isdigit(PC1)) return parse_int(p, 1);
      break;
    case '-':
      if (isdigit(PC1)) return parse_int(p, -1);
      return parse_label(p);

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


static Obj parse_src(SS path, SS src, BM* e) {
  // caller must free e.
  Parser p = (Parser) {
    .path=path,
    .src=src,
    .sp=(Src_pos){.pos=0, .line=0, .col=0,},
    .e=NULL,
  };
  Mem m = parse_seq(&p, 0);
  Obj o;
  if (p.e) {
    o = ILLEGAL;
  }
  else if (p.sp.pos != p.src.len) {
    parse_error(&p, "parsing terminated early");
    o = ILLEGAL;
  }
  else {
    o = new_vec_HM(obj_ret_val(DO), m); // implicit top-level DO.
  }
  mem_dealloc(m);
  *e = cast(BM, p.e);
  return o;
}

