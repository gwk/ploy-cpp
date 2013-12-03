// Copyright 2011 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "07-types.c"


// main parser object holds the input string, parse state, and source location info.
typedef struct {
  SS  src;  // input string.
  SS  path; // input path for error reporting.
  Int pos;
  Int line;
  Int col;
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


 __attribute__((format (printf, 2, 3)))
static Obj parse_error(Parser* p, BC fmt, ...) {
  va_list args;
  va_start(args, fmt);
  BM msg;
  Int msg_len = vasprintf(&msg, fmt, args);
  va_end(args);
  check(msg_len >= 0, "parse_error allocation failed: %s", fmt);
  // e must be freed by parser owner.
  p->e = ss_src_loc_str(p->src, p->path, p->pos, 0, p->line, p->col, msg);
  free(msg);
  return VOID;
}


#define parse_check(condition, fmt, ...) \
if (!(condition)) return parse_error(p, fmt, ##__VA_ARGS__)


#define PP  (p->pos < p->src.len)
#define PP1 (p->pos < p->src.len - 1)
#define PP2 (p->pos < p->src.len - 2)

#define PC  p->src.b.c[p->pos]
#define PC1 p->src.b.c[p->pos + 1]
#define PC2 p->src.b.c[p->pos + 2]

#define P_ADV(n) p->pos += n; p->col += n
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
  BM end;
  U64 u = strtoull(p->src.b.c + p->pos, &end, base);
  int en = errno;
  if (en) {
    parse_error(p, "malformed number literal: %s", strerror(en));
    return 0;
  }
  Int n = end - p->src.b.c;
  P_ADV(n);
  if (PP && !char_is_atom_term(PC)) {
    parse_error(p, "invalid number literal terminator: %c", PC);
    return 0;
  }
  return u;
}


static Obj parse_Uns(Parser* p) {
  return new_uns(parse_U64(p));
}


static Obj parse_Int(Parser* p, Int sign) {
  assert(PC == '-' || PC == '+');
  P_ADV1;
  U64 u = parse_U64(p);
  parse_check(u <= max_Int, "signed number literal is too large");
  return new_int((I64)u * sign);
}


static Obj parse_Sym(Parser* p) {
  assert(PC == '_' || isalpha(PC));
  Int pos = p->pos;
  loop {
    P_ADV1;
    if (!PP) break;
    Char c = PC;
    if (!(c == '-' || c == '_' || isalnum(c))) break;
  }
  SS s = ss_slice(p->src, pos, p->pos);
  return new_sym(s);
}


static Obj parse_Str(Parser* p, Char q) {
  assert(PC == q);
  Int pos_open = p->pos; // for error reporting.
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
      p->pos = pos_open; // better error message.
      return parse_error(p, "unterminated string literal");
    }
    Char c = PC;
    if (escape) {
      escape = false;
      Char ce = c;
      switch (c) {
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
  return new_data(ss_mk(s.b, i));
}


static Obj parse_expr(Parser* p);


static Bool parse_space(Parser* p) {
  while (PP) {
    Char c = PC;
    switch (c) {
      case ' ':
        P_ADV1;
        continue;
      case '\n':
        p->pos++;
        p->line++;
        p->col = 0;
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


// return Mem must be freed.
static Mem parse_seq(Parser* p) {
  Array a = array0;
  while (parser_has_next_expr(p)) {
    Obj o = parse_expr(p);
    Obj v = new_v2(o, E);
    array_append(&a, v);
  }
  return a.mem;
}


// return Mem must be freed.
static Mem parse_blocks(Parser* p) {
  Array a = array0;
  while (parser_has_next_expr(p)) {
    Char c = PC;
    Obj o;
    if (c == '|') {
      P_ADV1;
      Mem m = parse_seq(p);
      o = new_vec(m);
    }
    else {
      o = parse_expr(p);
    }
    array_append(&a, o);
  }
  return a.mem;
}


#define P_ADV_TERM(t) \
if (!parse_terminator(p, t)) { \
  mem_free(m); \
  return VOID; \
}


static Obj parse_call(Parser* p) {
  P_ADV1;
  Mem m = parse_seq(p);
  P_ADV_TERM(')');
  Obj c = new_chain(m);
  mem_free(m);
  return c;
}


static Obj parse_expa(Parser* p) {
  P_ADV1;
  Mem m = parse_seq(p);
  P_ADV_TERM('>');
  Obj c = new_chain(m);
  mem_free(m);
  Obj e = new_v2(EXPA, c);
  return e;
}


static Obj parse_vec(Parser* p) {
  P_ADV1;
  Mem m = parse_seq(p);
  P_ADV_TERM('}');
  Obj v = new_vec(m);
  mem_free(m);
  Obj q = new_v2(QUO, v);
  return q;
}


static Obj parse_chain(Parser* p) {
  P_ADV1;
  Mem m = parse_blocks(p);
  P_ADV_TERM(']');
  Obj c = new_chain(m);
  mem_free(m);
  Obj q = new_v2(QUO, c);
  return q;
}


static Obj parse_qua(Parser* p) {
  assert(PC == '`');
  P_ADV1;
  Obj o = parse_expr(p);
  return new_v2(QUA, o);
}


// parse an expression.
static Obj parse_expr(Parser* p) {
  Char c = PC;
  switch (c) {
    case '(':   return parse_call(p);
    case '<':   return parse_expa(p);
    case '{':   return parse_vec(p);
    case '[':   return parse_chain(p);
    case '`':   return parse_qua(p);
    case '\'':  return parse_Str(p, '\'');
    case '"':   return parse_Str(p, '"');
    case '+':
      if (isdigit(PC1)) return parse_Int(p, 1);
      return VOID;
    case '-':
      if (isdigit(PC1)) return parse_Int(p, -1);
      return VOID;

  }
  if (isdigit(c)) {
    return parse_Uns(p);
  }
  if (c == '_' || isalpha(c)) {
    return parse_Sym(p);
  }
  return parse_error(p, "unexpected character");
}


// caller must free e.
static Obj parse_ss(SS path, SS src, BM* e) {
  Parser p = (Parser) {
    .path=path,
    .src=src,
    .pos=0,
    .line=0,
    .col=0,
    .e=NULL,
  };
  Mem m = parse_seq(&p);
  Obj o;
  if (p.e) {
    o = VOID;
  }
  else if (p.pos != p.src.len) {
    parse_error(&p, "parsing terminated early");
    o = VOID;
  }
  else {
    Obj c = new_chain(m);
    o = new_v2(DO, c);
  }
  mem_free(m);
  *e = (BM)p.e; // TODO: fix this cast.
  return o;
}

