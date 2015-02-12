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
  Vector<Src_pos> start_positions;
  Parser(Dict* _locs, Obj _path, Obj _src, Str _s, Src_pos _pos):
  locs(_locs), path(_path), src(_src), s(_s), pos(_pos) {}
};


#define DEF_POS Src_pos pos = p.pos


#if VERBOSE_PARSE
static void parse_err_prefix(Parser& p) {
  fprintf(stderr, "%.*s:%ld:%ld (%ld): ",
    FMT_STR(data_str(p.path)), p.pos.line + 1, p.pos.col + 1, p.pos.off);
  if (p.e) errF("\nerror: %s\nobj:  ", p.e);
}

#define parse_errFL(fmt, ...) { parse_err_prefix(p); errFL(fmt, ##__VA_ARGS__) }
#endif


#define fmt_parse_error(fmt, ...) { \
  CharsM pos_info = str_src_loc(p.path.data_str(), p.pos.line, p.pos.col); \
  errFL("%s " fmt, pos_info, ##__VA_ARGS__); \
  raw_dealloc(pos_info, ci_Chars); \
  CharsM underline = str_src_underline(p.s, p.pos.off, 0, p.pos.col); \
  err(underline); \
  raw_dealloc(underline, ci_Chars); \
  fail(); \
}

#define parse_error(fmt, ...) { \
  fmt_parse_error("parse error: " fmt, ##__VA_ARGS__); \
  fail(); \
}


#define parse_check(condition, fmt, ...) \
if (!(condition)) parse_error(fmt, ##__VA_ARGS__)


#define PP  (p.pos.off < p.s.len)
#define PP1 (p.pos.off < p.s.len - 1)

#define PC  p.s.chars[p.pos.off]
#define PC1 p.s.chars[p.pos.off + 1]

#define P_ADV(n, ...) { p.pos.off += n; p.pos.col += n; if (!PP) {__VA_ARGS__;}; } 

#define P_CHARS (p.s.chars + p.pos.off)


static Bool char_is_seq_term(Char c) {
  // character terminates a syntactic sequence.
  return c == ')' || c == ']' || c == '}' || c == '>';
}


static Bool char_is_atom_term(Char c) {
  // character terminates an atom.
  return c == '|' || c == ':' || c == '=' || isspace(c) || char_is_seq_term(c);
}


static U64 parse_U64(Parser& p) {
  Char c = PC;
  I32 base = 0;
  if (c == '0' && PP1) {
    Char c1 = PC1;
    switch (c1) {
      case 'b': base = 2;   break;
      case 'q': base = 4;   break;
      case 'o': base = 8;   break;
      case 'd': base = 10;  break;
      case 'x': base = 16;  break;
    }
    if (base) {
      P_ADV(2, parse_error("incomplete number literal"); return 0);
    } else {
      base = 10;
    }
  }
  Chars start = p.s.chars + p.pos.off;
  CharsM end;
  // TODO: this appears unsafe if source string is not null-terminated.
  U64 u = strtoull(start, &end, base);
  int en = errno;
  parse_check(!en, "malformed number literal: %s", strerror(en));
  assert(end > start); // strtoull man page is a bit confusing as to the precise semantics.
  Int n = end - start;
  assert(p.pos.off + n <= p.s.len);
  P_ADV(n, return u);
  parse_check(char_is_atom_term(PC), "invalid number literal terminator: %c", PC);
  return u;
}


static Obj parse_uns(Parser& p) {
  U64 u = parse_U64(p);
  return Obj::with_U64(u);
}


static Obj parse_Int(Parser& p, Int sign) {
  assert(PC == '-' || PC == '+');
  P_ADV(1, parse_error("incomplete signed number literal"));
  U64 u = parse_U64(p);
  parse_check(u <= max_Int, "signed number literal is too large");
  return Obj::with_Int(Int(u) * sign);
}


static Obj parse_Sym(Parser& p) {
  assert(PC == '_' || isalpha(PC));
  Int off = p.pos.off;
  loop {
    P_ADV(1, break);
    Char c = PC;
    if (!(c == '-' || c == '_' || isalnum(c))) break;
  }
  Str s = str_slice(p.s, off, p.pos.off);
  return Obj::Sym(s, true);
}


static Obj parse_sub_expr(Parser& p);

static Obj parse_Comment(Parser& p) {
  DEF_POS;
  assert(PC == '#');
  P_ADV(1, parse_error("unterminated comment (add newline)"));
  if (PC == '#') { // double-hash comments out the following expression.
    P_ADV(1, parse_error("expression comment requires sub-expression"));
    Obj expr = parse_sub_expr(p);
    // first field indicates expression comment.
    return Obj::Cmpd(t_Comment.ret(), s_true.ret_val(), expr);
  }
  // otherwise comment out a single line.
  while (PC == ' ') {
    P_ADV(1, p.pos = pos; parse_error("unterminated comment (add newline)"));
  };
  Int off_start = p.pos.off;
  while (PC != '\n') {
    P_ADV(1, p.pos = pos; parse_error("unterminated comment (add newline)"));
  }
  Str s = str_slice(p.s, off_start, p.pos.off);
  Obj d = Obj::Data(s);
  return Obj::Cmpd(t_Comment.ret(), s_false.ret_val(), d);
}


static Obj parse_Data(Parser& p, Char q) {
  DEF_POS;
  assert(PC == q);
  String s;
  Bool escape = false;
  loop {
    P_ADV(1, p.pos = pos; parse_error("unterminated string literal"));
    Char c = PC;
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
  assert(PC == '`');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Quo.ret(), o);
}


static Obj parse_Qua(Parser& p) {
  assert(PC == '~');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Qua.ret(), o);
}


static Obj parse_Unq(Parser& p) {
  assert(PC == ',');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Unq.ret(), o);
}


static Obj parse_Bang(Parser& p) {
  assert(PC == '!');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Bang.ret(), o);
}


static Obj parse_Splice(Parser& p) {
  assert(PC == '*');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  return Obj::Cmpd(t_Splice.ret(), o);
}


static Obj parse_Label(Parser& p) {
  P_ADV(1, parse_error("incomplete label name"));
  Obj name = parse_sub_expr(p);
  Char c = PC;
  Obj type;
  if (c == ':') {
    P_ADV(1, parse_error("incomplete label type"));
    type = parse_sub_expr(p);
  } else {
    type = s_nil.ret_val();
  }
  Obj expr;
  if (PC == '=') {
    P_ADV(1, parse_error("incomplete label expr"));
    expr = parse_sub_expr(p);
  } else {
    expr = s_void.ret_val();
  }
  return Obj::Cmpd(t_Label.ret(), name, type, expr);
}


static Obj parse_Variad(Parser& p) {
  P_ADV(1, parse_error("incomplete variad expression"));
  Obj expr = parse_sub_expr(p);
  Char c = PC;
  Obj type;
  if (c == ':') {
    P_ADV(1, parse_error("incomplete variad type"));
    type = parse_sub_expr(p);
  } else {
    type = s_nil.ret_val();
  }
  return Obj::Cmpd(t_Variad.ret(), expr, type);
}


static Obj parse_Accessor(Parser& p) {
  P_ADV(1, parse_error("incomplete accessor"));
  Obj expr = parse_sub_expr(p);
  return Obj::Cmpd(t_Accessor.ret(), expr);
}


static Obj parse_Mutator(Parser& p) {
  P_ADV(2, parse_error("incomplete mutator"));
  Obj expr = parse_sub_expr(p);
  return Obj::Cmpd(t_Mutator.ret(), expr);
}


static Bool parse_space(Parser& p) {
  // return true if parsing can continue.
  while (PP) {
    Char c = PC;
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
        parse_error("illegal tab character");
      default:
        parse_check(!isspace(c), "illegal space character: '%c' (%02x)", c, c);
        return true;
    }
  }
  return false; // EOS.
}


static Obj parse_expr(Parser& p);

static Obj parse_exprs(Parser& p) {
  List l;
  while (parse_space(p) && !char_is_seq_term(PC)) {
    Obj o = parse_expr(p);
    l.append(o);
  }
  Obj a = Cmpd_from_Array(t_Arr_Expr.ret(), l.array());
  l.dealloc();
  return a;
}


static Obj parse_seq(Parser& p, Obj type, Chars chars_open, Char char_close) {
  p.start_positions.push_back(p.pos);
  while (*chars_open) {
    assert(PC == *chars_open);
    chars_open++;
    P_ADV(1, parse_error("expected terminator: '%c'", char_close));
  }
  Obj a = parse_exprs(p);
  parse_check(PC == char_close, "expected terminator: '%c'", char_close);
  P_ADV(1);
  p.start_positions.pop_back();
  if (type == t_Arr_Expr) {
    return a;
  }
  return Obj::Cmpd(type.ret(), a);
}


// parse an expression.
static Obj parse_expr_dispatch(Parser& p) {
  Char c = PC;
  switch (c) {
    case '<':   return parse_seq(p, t_Expand,   "<", '>');
    case '(':   return parse_seq(p, t_Call,     "(", ')');
    case '{':   return parse_seq(p, t_Arr_Expr, "{", '}');
    case '[':   return parse_seq(p, t_Syn_seq,  "[", ']');
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
      if (PP1 && PC1 == '=') return parse_Mutator(p);
      return parse_Accessor(p);
    case '+':
      if (PP1 && isdigit(PC1)) return parse_Int(p, 1);
      break;
    case '-':
      if (PP1 && isdigit(PC1)) return parse_Int(p, -1);
      return parse_Label(p);

  }
  if (isdigit(c)) {
    return parse_uns(p);
  }
  if (c == '_' || isalpha(c)) {
    return parse_Sym(p);
  }
  parse_error("unexpected character: '%s'", char_repr(c));
}


static Obj parse_expr(Parser& p) {
  DEF_POS;
  Obj expr = parse_expr_dispatch(p);
#if VERBOSE_PARSE
  parse_errFL("%o", expr);
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
  parse_check(PP, "expected sub-expression");
  return parse_expr(p);
}


static Obj parse_src(Dict& src_locs, Obj path, Obj src) {
  Parser p = Parser(&src_locs, path, src, src.data_str(), Src_pos(0, 0, 0));
  Obj a = parse_exprs(p);
  if (p.pos.off != p.s.len) {
    parse_error("parsing terminated early");
  }
  return a;
}
