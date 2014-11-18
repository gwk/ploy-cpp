/// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "17-fmt.h"


struct Src_pos {
  Int off;
  Int line;
  Int col;
  Src_pos(Int o, Int l, Int c): off(o), line(l), col(c) {}
};


struct Parser {
  Dict* locs; // maps parsed expressions to Src-locs.
  Obj path;
  Obj src;
  Str s;
  Src_pos pos;
  CharsM e; // error message.
  Parser(Dict* _locs, Obj _path, Obj _src, Str _s, Src_pos _pos, CharsM _e):
  locs(_locs), path(_path), src(_src), s(_s), pos(_pos), e(_e) {}
};


#define DEF_POS Src_pos pos = p.pos


#if VERBOSE_PARSE
static void parse_err_prefix(Parser& p) {
  fprintf(stderr, "%.*s:%ld:%ld (%ld): ",
    FMT_STR(data_str(p.path)), p.pos.line + 1, p.pos.col + 1, p.pos.off);
  if (p.e) errF("\nerror: %s\nobj:  ", p.e);
}

#define parse_errFL(fmt, ...) parse_err_prefix(p); errFL(fmt, ##__VA_ARGS__)
#endif

__attribute__((format (printf, 2, 3)))
static Obj parse_error(Parser& p, Chars fmt, ...) {
  assert(!p.e);
  va_list args;
  va_start(args, fmt);
  CharsM msg;
  Int msg_len = vasprintf(&msg, fmt, args);
  va_end(args);
  check(msg_len >= 0, "parse_error allocation failed: %s", fmt);
  // parser owner must call raw_dealloc on e.
  p.e = str_src_loc_str(p.path.data_str(), p.s, p.pos.off, 0, p.pos.line, p.pos.col, msg);
  free(msg); // matches vasprintf above.
  return obj0;
}


#define parse_check(condition, fmt, ...) \
if (!(condition)) return parse_error(p, fmt, ##__VA_ARGS__)


#define PP  (p.pos.off < p.s.len)
#define PP1 (p.pos.off < p.s.len - 1)

#define PC  p.s.chars[p.pos.off]
#define PC1 p.s.chars[p.pos.off + 1]

#define P_ADV(n, ...) { p.pos.off += n; p.pos.col += n; if (!PP) {__VA_ARGS__;}; } 

#define P_CHARS (p.s.chars + p.pos.off)


static Bool char_is_seq_term(Char c) {
  // character terminates a syntactic sequence.
  return
  c == ')' ||
  c == ']' ||
  c == '}' ||
  c == '>';
}


static Bool char_is_atom_term(Char c) {
  // character terminates an atom.
  return c == '|' || c == ':' || c == '=' || isspace(c) || char_is_seq_term(c);
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


static Bool parser_has_next_expr(Parser& p) {
  return parse_space(p) && !char_is_seq_term(PC);
}


static Obj parse_expr(Parser& p);

static Array parse_exprs(Parser& p, Char term) {
  // caller must call array.rel_dealloc or array.dealloc on returned Array.
  // term is a the expected terminator character (e.g. closing paren).
  List a;
  while (parser_has_next_expr(p)) {
    if (term && PC == term) break;
    Obj o = parse_expr(p);
    if (p.e) {
      assert(!o.vld());
      break;
    }
    a.append(o);
  }
  if (p.e) {
    a.rel_els_dealloc();
    return Array();
  }
  return a.array();
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
      P_ADV(2, parse_error(p, "incomplete number literal"); return 0);
    } else {
      base = 10;
    }
  }
  Chars start = p.s.chars + p.pos.off;
  CharsM end;
  // TODO: this appears unsafe if source string is not null-terminated.
  U64 u = strtoull(start, &end, base);
  int en = errno;
  if (en) {
    parse_error(p, "malformed number literal: %s", strerror(en));
    return 0;
  }
  assert(end > start); // strtoull man page is a bit confusing as to the precise semantics.
  Int n = end - start;
  assert(p.pos.off + n <= p.s.len);
  P_ADV(n, return u);
  if (!char_is_atom_term(PC)) {
    parse_error(p, "invalid number literal terminator: %c", PC);
    return 0;
  }
  return u;
}


static Obj parse_uns(Parser& p) {
  U64 u = parse_U64(p);
  if (p.e) return obj0;
  return Obj::with_U64(u);
}


static Obj parse_Int(Parser& p, Int sign) {
  assert(PC == '-' || PC == '+');
  P_ADV(1, return parse_error(p, "incomplete signed number literal"));
  U64 u = parse_U64(p);
  if (p.e) return obj0;
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
  return Obj::Sym(s);
}


static Obj parse_sub_expr(Parser& p);

static Obj parse_Comment(Parser& p) {
  DEF_POS;
  assert(PC == '#');
  P_ADV(1, return parse_error(p, "unterminated comment (add newline)"));
  if (PC == '#') { // double-hash comments out the following expression.
    P_ADV(1, return parse_error(p, "expression comment requires sub-expression"));
    Obj expr = parse_sub_expr(p);
    if (p.e) return obj0;
    // first field indicates expression comment.
    return Obj::Cmpd(t_Comment.ret(), s_true.ret_val(), expr);
  }
  // otherwise comment out a single line.
  while (PC == ' ') {
    P_ADV(1, p.pos = pos; return parse_error(p, "unterminated comment (add newline)"));
  };
  Int off_start = p.pos.off;
  while (PC != '\n') {
    P_ADV(1, p.pos = pos; return parse_error(p, "unterminated comment (add newline)"));
  }
  Str s = str_slice(p.s, off_start, p.pos.off);
  Obj d = Obj::Data(s);
  return Obj::Cmpd(t_Comment.ret(), s_false.ret_val(), d);
}


static Obj parse_Data(Parser& p, Char q) {
  DEF_POS;
  assert(PC == q);
  Int cap = size_min_alloc;
  CharsM chars = chars_alloc(cap);
  Int len = 0;
  Bool escape = false;
#define APPEND(c) { len = chars_append(&chars, &cap, len, c); }
  loop {
    P_ADV(1,
      p.pos = pos; chars_dealloc(chars); return parse_error(p, "unterminated string literal"));
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
        default: APPEND('\\'); // not a valid escape code, so add the leading backslash.
      }
      APPEND(ce);
    } else {
      if (c == q) break;
      if (c == '\\') escape = true;
      else APPEND(c);
    }
  }
  #undef APPEND
  P_ADV(1); // past closing quote.
  Obj d = Obj::Data(Str(len, len ? chars: null));
  chars_dealloc(chars);
  return d;
}


static Obj parse_Quo(Parser& p) {
  assert(PC == '`');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  if (p.e) return obj0;
  return Obj::Cmpd(t_Quo.ret(), o);
}


static Obj parse_Qua(Parser& p) {
  assert(PC == '~');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  if (p.e) return obj0;
  return Obj::Cmpd(t_Qua.ret(), o);
}


static Obj parse_Unq(Parser& p) {
  assert(PC == ',');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  if (p.e) return obj0;
  return Obj::Cmpd(t_Unq.ret(), o);
}


static Obj parse_Bang(Parser& p) {
  assert(PC == '!');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  if (p.e) return obj0;
  return Obj::Cmpd(t_Bang.ret(), o);
}


static Obj parse_Splice(Parser& p) {
  assert(PC == '*');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  if (p.e) return obj0;
  return Obj::Cmpd(t_Splice.ret(), o);
}


static Obj parse_Label(Parser& p) {
  P_ADV(1, return parse_error(p, "incomplete label name"));
  Obj name = parse_sub_expr(p);
  if (p.e) {
    assert(!name.vld());
    return obj0;
  }
  Char c = PC;
  Obj type;
  if (c == ':') {
    P_ADV(1, return parse_error(p, "incomplete label type"));
    type = parse_sub_expr(p);
    if (p.e) {
      name.rel();
      assert(!type.vld());
      return obj0;
    }
  } else {
    type = s_nil.ret_val();
  }
  Obj expr;
  if (PC == '=') {
    P_ADV(1, return parse_error(p, "incomplete label expr"));
    expr = parse_sub_expr(p);
    if (p.e) {
      name.rel();
      type.rel();
      assert(!expr.vld());
      return obj0;
    }
  } else {
    expr = s_void.ret_val();
  }
  return Obj::Cmpd(t_Label.ret(), name, type, expr);
}


static Obj parse_Variad(Parser& p) {
  P_ADV(1, return parse_error(p, "incomplete variad expression"));
  Obj expr = parse_sub_expr(p);
  if (p.e) {
    assert(!expr.vld());
    return obj0;
  }
  Char c = PC;
  Obj type;
  if (c == ':') {
    P_ADV(1, return parse_error(p, "incomplete variad type"));
    type = parse_sub_expr(p);
    if (p.e) {
      expr.rel();
      assert(!type.vld());
      return obj0;
    }
  } else {
    type = s_nil.ret_val();
  }
  return Obj::Cmpd(t_Variad.ret(), expr, type);
}


static Obj parse_Accessor(Parser& p) {
  P_ADV(1, return parse_error(p, "incomplete accessor"));
  Obj expr = parse_sub_expr(p);
  if (p.e) {
    assert(!expr.vld());
    return obj0;
  }
  return Obj::Cmpd(t_Accessor.ret(), expr);
}


static Obj parse_Mutator(Parser& p) {
  P_ADV(2, return parse_error(p, "incomplete mutator"));
  Obj expr = parse_sub_expr(p);
  if (p.e) {
    assert(!expr.vld());
    return obj0;
  }
  return Obj::Cmpd(t_Mutator.ret(), expr);
}


static Bool parse_terminator(Parser& p, Char t) {
  if (PC != t) {
    parse_error(p, "expected terminator: '%c'", t);
    return false;
  }
  P_ADV(1);
  return true;
}


#define P_CONSUME_TERMINATOR(t) \
if (p.e || !parse_terminator(p, t)) { \
  a.rel_els_dealloc(); \
  return obj0; \
}


static Obj parse_Expand(Parser& p) {
  P_ADV(1, return parse_error(p, "unterminated expand"));
  Array a = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR('>');
  Obj e = Obj::Cmpd(t_Expand.ret(), Cmpd_from_Array(t_Arr_Expr.ret(), a));
  a.dealloc();
  return e;
}


static Obj parse_Call(Parser& p) {
  P_ADV(1, return parse_error(p, "unterminated call"));
  Array a = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR(')');
  Obj c = Obj::Cmpd(t_Call.ret(), Cmpd_from_Array(t_Arr_Expr.ret(), a));
  a.dealloc();
  return c;
}


static Obj parse_struct(Parser& p) {
  P_ADV(1, return parse_error(p, "unterminated constructor"));
  Array a = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR('}');
  Obj s = Obj::Cmpd(t_Syn_struct.ret(), Cmpd_from_Array(t_Arr_Expr.ret(), a));
  a.dealloc();
  return s;
}


static Obj parse_seq(Parser& p) {
  P_ADV(1, return parse_error(p, "unterminated sequence"));
  Array a = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR(']');
  Obj s = Obj::Cmpd(t_Syn_seq.ret(), Cmpd_from_Array(t_Arr_Expr.ret(), a));
  a.dealloc();
  return s;
}


// parse an expression.
static Obj parse_expr_dispatch(Parser& p) {
  Char c = PC;
  switch (c) {
    case '<':   return parse_Expand(p);
    case '(':   return parse_Call(p);
    case '{':   return parse_struct(p);
    case '[':   return parse_seq(p);
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
  return parse_error(p, "unexpected character: '%s'", char_repr(c));
}


static Obj parse_expr(Parser& p) {
  DEF_POS;
  Obj expr = parse_expr_dispatch(p);
#if VERBOSE_PARSE
  parse_errFL("%o", expr);
#endif
  if (!p.e && expr.is_ref()) {
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


static Obj parse_src(Dict& src_locs, Obj path, Obj src, CharsM* e) {
  // caller must free e.
  Parser p = Parser(&src_locs, path, src, src.data_str(), Src_pos(0, 0, 0), null);
  Array a = parse_exprs(p, 0);
  Obj o;
  if (p.e) {
    assert(!a.len());
    o = obj0;
  } else if (p.pos.off != p.s.len) {
    o = parse_error(p, "parsing terminated early");
    a.rel_els_dealloc();
  } else {
    o = Cmpd_from_Array(t_Arr_Expr.ret(), a);
    a.dealloc();
  }
  *e = p.e;
  return o;
}