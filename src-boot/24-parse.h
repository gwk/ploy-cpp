/// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "23-write-repr.h"


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


static void parse_err(Parser* p) {
  fprintf(stderr, "%.*s:%ld:%ld (%ld): ",
    FMT_STR(p->path), p->sp.line + 1, p->sp.col + 1, p->sp.pos);
  if (p->e) errF("\nerror: %s\nobj:  ", p->e);
}


UNUSED_FN static void parse_errL(Parser* p) {
  parse_err(p);
  err_nl();
}


 __attribute__((format (printf, 2, 3)))
static Obj parse_error(Parser* p, Chars_const fmt, ...) {
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

#define P_ADV(n, ...) { p->sp.pos += n; p->sp.col += n; if (!PP) {__VA_ARGS__;}; } 

#define P_CHARS (p->src.chars + p->sp.pos)


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


static Bool parse_space(Parser* p) {
  // return true if parsing can continue.
  while (PP) {
    Char c = PC;
    switch (c) {
      case ' ':
        P_ADV(1, break);
       break ;
      case '\n':
        p->sp.pos++;
        p->sp.line++;
        p->sp.col = 0;
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


static Bool parser_has_next_expr(Parser* p) {
  return parse_space(p) && !char_is_seq_term(PC);
}


static Obj parse_expr(Parser* p);

static Mem parse_exprs(Parser* p, Char term) {
  // caller must call mem_rel_dealloc or mem_dealloc on returned Mem.
  // term is a the expected terminator character (e.g. closing paren).
  Array a = array0;
  while (parser_has_next_expr(p)) {
    if (term && PC == term) break;
    Obj o = parse_expr(p);
    if (p->e) {
      assert(is(o, obj0));
      break;
    }
    array_append(&a, o);
  }
  if (p->e) {
    mem_rel_dealloc(a.mem);
    return mem0;
  }
  return a.mem;
}



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
      case 'x': base = 16;  break;
    }
    if (base) {
      P_ADV(2, parse_error(p, "incomplete number literal"); return 0);
    } else {
      base = 10;
    }
  }
  Chars_const start = p->src.chars + p->sp.pos;
  Chars end;
  // TODO: this appears unsafe; what if strtoull runs off the end of the string?
  U64 u = strtoull(start, &end, base);
  int en = errno;
  if (en) {
    parse_error(p, "malformed number literal: %s", strerror(en));
    return 0;
  }
  assert(end > start); // strtoull man page is a bit confusing as to the precise semantics.
  Int n = end - start;
  assert(p->sp.pos + n <= p->src.len);
  P_ADV(n, return u);
  if (!char_is_atom_term(PC)) {
    parse_error(p, "invalid number literal terminator: %c", PC);
    return 0;
  }
  return u;
}


static Obj parse_uns(Parser* p) {
  U64 u = parse_U64(p);
  if (p->e) return obj0;
  return int_new_from_uns(u);
}


static Obj parse_int(Parser* p, Int sign) {
  assert(PC == '-' || PC == '+');
  P_ADV(1, return parse_error(p, "incomplete signed number literal"));
  U64 u = parse_U64(p);
  if (p->e) return obj0;
  parse_check(u <= max_Int, "signed number literal is too large");
  return int_new((I64)u * sign);
}


static Obj parse_sym(Parser* p) {
  assert(PC == '_' || isalpha(PC));
  Int pos = p->sp.pos;
  loop {
    P_ADV(1, break);
    Char c = PC;
    if (!(c == '-' || c == '_' || isalnum(c))) break;
  }
  Str s = str_slice(p->src, pos, p->sp.pos);
  return sym_new(s);
}


static Obj parse_sub_expr(Parser* p);

static Obj parse_comment(Parser* p) {
  assert(PC == '#');
  P_ADV(1, return parse_error(p, "unterminated comment (add newline)"));
  if (PC == '#') { // double-hash comments out the following expression.
    P_ADV(1, return parse_error(p, "expression comment requires sub-expression"));
    Obj expr = parse_sub_expr(p);
    if (p->e) return obj0;
    // first field indicates expression comment.
    return struct_new2(rc_ret(t_Comment), rc_ret_val(s_true), expr);
  }
  // otherwise comment out a single line.
  Src_pos sp_report = p->sp;
  while (PC == ' ') {
    P_ADV(1, p->sp = sp_report; return parse_error(p, "unterminated comment (add newline)"));
  };
  Int pos_start = p->sp.pos;
  while (PC != '\n') {
    P_ADV(1, p->sp = sp_report; return parse_error(p, "unterminated comment (add newline)"));
  }
  Str s = str_slice(p->src, pos_start, p->sp.pos);
  Obj d = data_new_from_str(s);
  return struct_new2(rc_ret(t_Comment), rc_ret_val(s_false), d);
}


static Obj parse_data(Parser* p, Char q) {
  assert(PC == q);
  Src_pos sp = p->sp; // for error reporting.
  Int cap = size_min_alloc;
  Chars chars = chars_alloc(cap);
  Int len = 0;
  Bool escape = false;
#define APPEND(c) { len = chars_append(&chars, &cap, len, c); }
  loop {
    P_ADV(1, p->sp = sp; chars_dealloc(chars); return parse_error(p, "unterminated string literal"));
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
  Obj d = data_new_from_str(str_mk(len, chars));
  chars_dealloc(chars);
  return d;
}


static Obj parse_quo(Parser* p) {
  assert(PC == '`');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  if (p->e) return obj0;
  return struct_new1(rc_ret(t_Quo), o);
}


static Obj parse_qua(Parser* p) {
  assert(PC == '~');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  if (p->e) return obj0;
  return struct_new1(rc_ret(t_Qua), o);
}


static Obj parse_unq(Parser* p) {
  assert(PC == ',');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  if (p->e) return obj0;
  return struct_new1(rc_ret(t_Unq), o);
}


static Obj parse_bang(Parser* p) {
  assert(PC == '!');
  P_ADV(1);
  Obj o = parse_sub_expr(p);
  if (p->e) return obj0;
  return struct_new1(rc_ret(t_Bang), o);
}


static Obj parse_label(Parser* p) {
  P_ADV(1, return parse_error(p, "incomplete label name"));
  Obj name = parse_sub_expr(p);
  if (p->e) {
    assert(is(name, obj0));
    return obj0;
  }
  Char c = PC;
  Obj type;
  if (c == ':') {
    P_ADV(1, return parse_error(p, "incomplete label type"));
    type = parse_sub_expr(p);
    if (p->e) {
      rc_rel(name);
      assert(is(type, obj0));
      return obj0;
    }
  } else {
    type = rc_ret_val(s_INFER_PAR);
  }
  Obj expr;
  if (PC == '=') {
    P_ADV(1, return parse_error(p, "incomplete label default"));
    expr = parse_sub_expr(p);
    if (p->e) {
      rc_rel(name);
      rc_rel(type);
      assert(is(expr, obj0));
      return obj0;
    }
  } else {
    expr = rc_ret_val(s_void);
  }
  return struct_new3(rc_ret(t_Label), name, type, expr);
}


static Obj parse_variad(Parser* p) {
  P_ADV(1, return parse_error(p, "incomplete variad expression"));
  Obj expr = parse_sub_expr(p);
  if (p->e) {
    assert(is(expr, obj0));
    return obj0;
  }
  Char c = PC;
  Obj type;
  if (c == ':') {
    P_ADV(1, return parse_error(p, "incomplete variad type"));
    type = parse_sub_expr(p);
    if (p->e) {
      rc_rel(expr);
      assert(is(type, obj0));
      return obj0;
    }
  } else {
    type = rc_ret_val(s_INFER_PAR);
  }
  return struct_new2(rc_ret(t_Variad), expr, type);
}


static Obj parse_accessor(Parser* p) {
  P_ADV(1, return parse_error(p, "incomplete accessor"));
  Obj expr = parse_sub_expr(p);
  if (p->e) {
    assert(is(expr, obj0));
    return obj0;
  }
  return struct_new1(rc_ret(t_Accessor), expr);
}


static Obj parse_mutator(Parser* p) {
  P_ADV(2, return parse_error(p, "incomplete mutator"));
  Obj expr = parse_sub_expr(p);
  if (p->e) {
    assert(is(expr, obj0));
    return obj0;
  }
  return struct_new1(rc_ret(t_Mutator), expr);
}


static Bool parse_terminator(Parser* p, Char t) {
  if (PC != t) {
    parse_error(p, "expected terminator: '%c'", t);
    return false;
  }
  P_ADV(1);
  return true;
}


#define P_CONSUME_TERMINATOR(t) \
if (p->e || !parse_terminator(p, t)) { \
  mem_rel_dealloc(m); \
  return obj0; \
}


static Obj parse_expand(Parser* p) {
  P_ADV(1, return parse_error(p, "unterminated expand"));
  Mem m = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR('>');
  Obj s = struct_new_M(rc_ret(t_Expand), m);
  mem_dealloc(m);
  return s;
}


static Obj parse_call(Parser* p) {
  P_ADV(1, return parse_error(p, "unterminated call"));
  Mem m = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR(')');
  Obj s = struct_new_M(rc_ret(t_Call), m);
  mem_dealloc(m);
  return s;
}


static Obj parse_struct(Parser* p) {
  P_ADV(1, return parse_error(p, "unterminated struct"));
  Bool typed = false;
  if (PC == ':') {
    P_ADV(1, return parse_error(p, "unterminated struct"));
    typed = true;
  }
  Src_pos sp = p->sp;
  Mem m = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR('}');
  if (typed && !m.len) {
    p->sp = sp;
    return parse_error(p, "typed struct requires a type expression");
  }
  Obj s;
  if (typed) {
    s = struct_new_M(rc_ret(t_Syn_struct_typed), m);
  } else {
    s = struct_new_M(rc_ret(t_Syn_struct), m);
  }
  mem_dealloc(m);
  return s;
}


static Obj parse_seq(Parser* p) {
  P_ADV(1, return parse_error(p, "unterminated sequence"));
  Bool typed = false;
  if (PC == ':') {
    P_ADV(1, return parse_error(p, "unterminated sequence"));
    typed = true;
  }
  Src_pos sp = p->sp;
  Mem m = parse_exprs(p, 0);
  P_CONSUME_TERMINATOR(']');
  if (typed && !m.len) {
    p->sp = sp;
    return parse_error(p, "typed sequence requires a type expression");
  }
  Obj s;
  if (typed) {
    s = struct_new_M(rc_ret(t_Syn_seq_typed), m);
  } else {
    s = struct_new_M(rc_ret(t_Syn_seq), m);
  }
  mem_dealloc(m);
  return s;
}


// parse an expression.
static Obj parse_expr_dispatch(Parser* p) {
  Char c = PC;
  switch (c) {
    case '<':   return parse_expand(p);
    case '(':   return parse_call(p);
    case '{':   return parse_struct(p);
    case '[':   return parse_seq(p);
    case '`':   return parse_quo(p);
    case '~':   return parse_qua(p);
    case ',':   return parse_unq(p);
    case '!':   return parse_bang(p);
    case '\'':  return parse_data(p, '\'');
    case '"':   return parse_data(p, '"');
    case '#':   return parse_comment(p);
    case '&':   return parse_variad(p);
    case '.':
      if (PP1 && PC1 == '=') return parse_mutator(p);
      return parse_accessor(p);
    case '+':
      if (PP1 && isdigit(PC1)) return parse_int(p, 1);
      break;
    case '-':
      if (PP1 && isdigit(PC1)) return parse_int(p, -1);
      return parse_label(p);

  }
  if (isdigit(c)) {
    return parse_uns(p);
  }
  if (c == '_' || isalpha(c)) {
    return parse_sym(p);
  }
  return parse_error(p, "unexpected character: '%s'", char_repr(c));
}


static Obj parse_expr(Parser* p) {
  Obj o = parse_expr_dispatch(p);
#if VERBOSE_PARSE
  parse_err(p);
  errFL("%o", o);
#endif
  return o;
}


static Obj parse_sub_expr(Parser* p) {
  parse_check(PP, "expected sub-expression");
  return parse_expr(p);
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
  } else if (p.sp.pos != p.src.len) {
    o = parse_error(&p, "parsing terminated early");
    mem_rel_dealloc(m);
  } else {
    o = struct_new_M(rc_ret(t_Mem_Expr), m);
    mem_dealloc(m);
  }
  *e = p.e;
  return o;
}
