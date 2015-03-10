/// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "18-fmt.h"


struct Src_pos {
  Int off;
  Int line;
  Int col;
  Src_pos(): off(-1), line(-1), col(-1) {}
  Src_pos(Int o, Int l, Int c): off(o), line(l), col(c) {}
};


struct Parser {
  Dict* locs; // maps parsed expressions to Src-locs.
  Obj path;
  Obj src;
  Str s;
  Src_pos pos;
  Parser(Dict* _locs, Obj _path, Obj _src, Str _s, Src_pos _pos):
  locs(_locs), path(_path), src(_src), s(_s), pos(_pos) {}
};


// save the current source pos for error reporting.
#define DEF_POS Src_pos pos = p.pos


#if VERBOSE_PARSE
static void parser_err_prefix(Parser& p) {
  fprintf(stderr, "%.*s:%ld:%ld (%ld): ",
    FMT_STR(data_str(p.path)), p.pos.line + 1, p.pos.col + 1, p.pos.off);
  if (p.e) errF("\nerror: %s\nobj:  ", p.e);
}

#define parser_errFL(fmt, ...) { parser_err_prefix(p); errFL(fmt, ##__VA_ARGS__) }
#endif


#define fmt_parser_error(pos, fmt, ...) { \
  CharsM pos_info = str_src_loc(p.path.data_str(), pos.line, pos.col); \
  errFL("%s " fmt, pos_info, ##__VA_ARGS__); \
  raw_dealloc(pos_info, ci_Chars); \
  CharsM underline = str_src_underline(p.s, pos.off, 0, pos.col); \
  err(underline); \
  raw_dealloc(underline, ci_Chars); \
}

#define parser_error(fmt, ...) { \
  fmt_parser_error(p.pos, "parse error: " fmt, ##__VA_ARGS__); \
  fail(); \
}

#define parser_check(condition, fmt, ...) \
if (!(condition)) parser_error(fmt, ##__VA_ARGS__)


#define P_SOME_AHEAD(n) (p.pos.off < p.s.len - (n + 1)) // 1 accounts for null terminator.
#define P_SOME P_SOME_AHEAD(0)

#define P_CHAR p.s.chars[p.pos.off]
#define P_NEXT p.s.chars[p.pos.off + 1]

#define P_ADV(n, ...) { p.pos.off += n; p.pos.col += n; if (!P_SOME) {__VA_ARGS__;}; }

#define P_CHARS (p.s.chars + p.pos.off)

// note: we specify "kw_len - 1" to allow matching a bare, terminal keyword;
// this allows the parser to diagnose the keyword as such, rather than treating it as a Sym.
#define P_MATCH(kw_len, kw_chars) \
(P_SOME_AHEAD(kw_len - 1) && \
  memcmp(P_CHARS, kw_chars, kw_len) == 0 && \
  !char_is_sym_subsequent(P_CHARS[kw_len]))


static Bool char_is_seq_term(Char c) {
  // character terminates a syntactic sequence.
  return c == ')' || c == ']' || c == '}' || c == '>' || c == ';' || c == '\0';
}


static Bool char_is_atom_term(Char c) {
  // character terminates an atom.
  return c == '|' || c == ':' || c == '=' || isspace(c) || char_is_seq_term(c);
}

static Bool char_is_sym_init(Char c) {
  return c == '_' || isalpha(c);
}

static Bool char_is_sym_subsequent(Char c) {
  return char_is_sym_init(c) || c == '-' || isdigit(c);
}


static U64 parse_U64(Parser& p) {
  Char c = P_CHAR;
  I32 base = 0;
  if (c == '0') { // ahead check is not necessary since this function requires null terminator.
    switch (P_NEXT) {
      case 'b': base = 2;   break;
      case 'q': base = 4;   break;
      case 'o': base = 8;   break;
      case 'd': base = 10;  break;
      case 'x': base = 16;  break;
    }
    if (base) {
      P_ADV(2, parser_error("incomplete number literal (EOS)"); return 0);
      parser_check(isdigit(P_CHAR), "incomplete number literal");
    } else {
      base = 10; // explicitly set base 10 so that leading 0 is not interpreted as octal.
    }
  }
  Chars start = p.s.chars + p.pos.off;
  CharsM end;
  // note: this is safe only because source string is guaranteed to be null-terminated.
  U64 u = strtoull(start, &end, base);
  int en = errno;
  parser_check(!en, "malformed number literal: %s", strerror(en));
  assert(end > start); // strtoull man page is a bit confusing as to the precise semantics.
  Int n = end - start;
  assert(p.pos.off + n <= p.s.len);
  P_ADV(n, return u);
  parser_check(char_is_atom_term(P_CHAR), "invalid number literal terminator: %c", P_CHAR);
  return u;
}


static Obj parse_uns(Parser& p) {
  U64 u = parse_U64(p);
  return Obj::with_U64(u);
}


static Obj parse_Int(Parser& p, Int sign) {
  assert(P_CHAR == '-' || P_CHAR == '+');
  P_ADV(1, parser_error("incomplete signed number literal"));
  U64 u = parse_U64(p);
  parser_check(u <= max_Int, "signed number literal is too large");
  return Obj::with_Int(Int(u) * sign);
}


static Obj parse_Sym(Parser& p) {
  assert(char_is_sym_init(P_CHAR));
  Int off = p.pos.off;
  loop {
    P_ADV(1, break);
    Char c = P_CHAR;
    if (!char_is_sym_subsequent(c)) break;
  }
  Str s = str_slice(p.s, off, p.pos.off);
  return Obj::Sym(s, true);
}


static Obj parse_sub_expr(Parser& p);

static Obj parse_Comment(Parser& p) {
  DEF_POS;
  assert(P_CHAR == '#');
  P_ADV(1, parser_error("unterminated comment (add newline)"));
  if (P_CHAR == '#') { // double-hash comments out the following expression.
    P_ADV(1, parser_error("expression comment requires sub-expression"));
    Obj expr = parse_sub_expr(p);
    // first field indicates expression comment.
    return Obj::Cmpd(t_Comment.ret(), s_true.ret_val(), expr);
  }
  // otherwise comment out a single line.
  while (P_CHAR == ' ') {
    P_ADV(1, p.pos = pos; parser_error("unterminated comment (add newline)"));
  };
  Int off_start = p.pos.off;
  while (P_CHAR != '\n') {
    P_ADV(1, p.pos = pos; parser_error("unterminated comment (add newline)"));
  }
  Str s = str_slice(p.s, off_start, p.pos.off);
  Obj d = Obj::Data(s);
  return Obj::Cmpd(t_Comment.ret(), s_false.ret_val(), d);
}


static Obj parse_Data(Parser& p, Char q) {
  DEF_POS;
  assert(P_CHAR == q);
  String s;
  Bool escape = false;
  loop {
    P_ADV(1, p.pos = pos; parser_error("unterminated string literal"));
    Char c = P_CHAR;
    if (escape) {
      escape = false;
      Char ce = c;
      switch (c) {
        case '0': ce = 0;     break; // null terminator.
        case 'a': ce = '\a';  break; // bell - BEL.
        case 'b': ce = '\b';  break; // backspace - BS.
        case 'f': ce = '\f';  break; // form feed - FF.
        case 'n': ce = '\n';  break; // line feed - LF.
        case 'r': ce = '\r';  break; // carriage return - CR.
        case 't': ce = '\t';  break; // horizontal tab - TAB.
        case 'v': ce = '\v';  break; // vertical tab - VT.
        case '\\':  break; // keep c as is.
        case '\'':  break;
        case '"':   break;
        case 'x': // ordinal escapes not yet implemented. \xx byte value.
        case 'u': // \uuuu unicode.
        case 'U': // \UUUUUU unicode. 6 or 8 chars? utf-8 is restricted to end at U+10FFFF.
        default: s.push_back('\\'); // not a valid escape code, so add the leading backslash.
      }
      s.push_back(ce);
    } else {
      if (c == q) break;
      if (c == '\\') escape = true;
      else s.push_back(c);
    }
  }
  P_ADV(1); // past closing quote.
  Obj d = Obj::Data(Str(s));
  return d;
}


static Obj parse_Quo(Parser& p) {
  assert(P_CHAR == '`');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Quo.ret(), o);
}


static Obj parse_Qua(Parser& p) {
  assert(P_CHAR == '~');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Qua.ret(), o);
}


static Obj parse_Unq(Parser& p) {
  assert(P_CHAR == ',');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Unq.ret(), o);
}


static Obj parse_Bang(Parser& p) {
  assert(P_CHAR == '!');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Bang.ret(), o);
}


static Obj parse_Splice(Parser& p) {
  assert(P_CHAR == '*');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Splice.ret(), o);
}


static Obj parse_Label(Parser& p) {
  P_ADV(1, parser_error("incomplete label name"));
  Obj name = parse_sub_expr(p);
  Char c = P_CHAR;
  Obj type;
  if (c == ':') {
    P_ADV(1, parser_error("incomplete label type"));
    type = parse_sub_expr(p);
  } else {
    type = s_nil.ret_val();
  }
  Obj expr;
  if (P_CHAR == '=') {
    P_ADV(1, parser_error("incomplete label expr"));
    expr = parse_sub_expr(p);
  } else {
    expr = s_void.ret_val();
  }
  return Obj::Cmpd(t_Label.ret(), name, type, expr);
}


static Obj parse_Variad(Parser& p) {
  P_ADV(1, parser_error("incomplete variad expression"));
  Obj expr = parse_sub_expr(p);
  Char c = P_CHAR;
  Obj type;
  if (c == ':') {
    P_ADV(1, parser_error("incomplete variad type"));
    type = parse_sub_expr(p);
  } else {
    type = s_nil.ret_val();
  }
  return Obj::Cmpd(t_Variad.ret(), expr, type);
}


static Obj parse_Accessor(Parser& p) {
  P_ADV(1, parser_error("incomplete accessor"));
  Obj expr = parse_sub_expr(p);
  return Obj::Cmpd(t_Accessor.ret(), expr);
}


static Obj parse_Mutator(Parser& p) {
  P_ADV(2, parser_error("incomplete mutator"));
  Obj expr = parse_sub_expr(p);
  return Obj::Cmpd(t_Mutator.ret(), expr);
}


static Bool parse_space(Parser& p) {
  // return true if parsing can continue.
  while (P_SOME) {
    Char c = P_CHAR;
    switch (c) {
      case ' ':
        P_ADV(1, break);
       break ;
      case '\n':
        p.pos.off++;
        p.pos.line++;
        p.pos.col = 0;
        break;
      case '\t':
        parser_error("illegal tab character");
      default:
        parser_check(!isspace(c), "illegal space character: '%c' (%02x)", c, c);
        return true;
    }
  }
  return false; // EOS.
}


static Obj parse_expr(Parser& p);

static Obj parse_exprs(Parser& p) {
  List l;
  while (parse_space(p) && !char_is_seq_term(P_CHAR)) {
    Obj o = parse_expr(p);
    l.append(o);
  }
  Obj a = Cmpd_from_Array(t_Arr_Expr.ret(), l.array());
  l.dealloc();
  return a;
}


static Obj parse_seq(Parser& p, Int len_open, Chars chars_open, Char char_close) {
  DEF_POS;
  P_ADV(len_open, parser_error("'%s' expression missing terminator: '%c'",
   chars_open, char_close));
  Obj a = parse_exprs(p);
  if (P_CHAR != char_close) {
    fmt_parser_error(pos, "parse error: expression starting here is missing terminator: '%c'",
      char_close);
    fmt_parser_error(p.pos, "expression appears to end here");
    fail();
  }
  P_ADV(1);
  return a;
}


static Obj parse_seq_wrapped(Parser& p, Obj type, Int len_open, Chars chars_open,
  Char char_close) {
  Obj a = parse_seq(p, len_open, chars_open, char_close);
  return Obj::Cmpd(type.ret(), a);
}


// parse an expression.
static Obj parse_expr_dispatch(Parser& p) {
  Char c = P_CHAR;
  switch (c) {
    case '{':   return parse_seq(p, 1, "{", '}');
    case '<':   return parse_seq_wrapped(p, t_Expand,   1, "<", '>');
    case '(':   return parse_seq_wrapped(p, t_Call,     1, "(", ')');
    case '[':   return parse_seq_wrapped(p, t_Syn_seq,  1, "[", ']');
    case '`':   return parse_Quo(p);
    case '~':   return parse_Qua(p);
    case ',':   return parse_Unq(p);
    case '!':   return parse_Bang(p);
    case '*':   return parse_Splice(p);
    case '\'':  return parse_Data(p, '\'');
    case '"':   return parse_Data(p, '"');
    case '#':   return parse_Comment(p);
    case '&':   return parse_Variad(p);
    case '.':
      if (P_SOME_AHEAD(1) && P_NEXT == '=') return parse_Mutator(p);
      return parse_Accessor(p);
    case '+':
      if (P_SOME_AHEAD(1) && isdigit(P_NEXT)) return parse_Int(p, 1);
      break;
    case '-':
      if (P_SOME_AHEAD(1) && isdigit(P_NEXT)) return parse_Int(p, -1);
      return parse_Label(p);
  }
  if (isdigit(c)) {
    return parse_uns(p);
  }
  if (c == '_') {
    return parse_Sym(p);
  }
  if (isalpha(c)) {
    if (P_MATCH(2, "if"))   return parse_seq_wrapped(p, t_If, 2, "if", ';');
    if (P_MATCH(4, "bind")) return parse_seq_wrapped(p, t_Bind, 4, "bind", ';');
    return parse_Sym(p);
  }
  parser_error("unexpected character: '%s'", char_repr(c));
}


static Obj parse_expr(Parser& p) {
  DEF_POS;
  Obj expr = parse_expr_dispatch(p);
#if VERBOSE_PARSE
  parser_errFL("%o", expr);
#endif
  if (expr.is_ref()) {
    Obj src_loc = Obj::Cmpd(t_Src_loc.ret(),
      p.path.ret(),
      p.src.ret(),
      Obj::with_Int(pos.off),
      Obj::with_Int(p.pos.off - pos.off),
      Obj::with_Int(pos.line),
      Obj::with_Int(pos.col));
    p.locs->insert(expr.ret(), src_loc); // dict owns k, v.
  }
  return expr;
}


static Obj parse_sub_expr(Parser& p) {
  parser_check(p.pos.off < p.s.len - 1, "expected sub-expression");
  return parse_expr(p);
}


static Obj parse_src(Dict& src_locs, Obj path, Obj src) {
  Parser p = Parser(&src_locs, path, src, src.data_str(), Src_pos(0, 0, 0));
  // input string is required to be null terminated, due to use of strtoull.
  assert(p.s.chars[p.s.len - 1] == '\0');
  Obj a = parse_exprs(p);
  if (P_SOME) {
    parser_error("parsing terminated early");
  }
  return a;
}
