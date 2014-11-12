// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// type objects.

#include "22-global.h"


struct Type {
  Head head;
  Int len;
  Obj name;
  Obj kind;
  Type(): head(obj0), len(0), name(obj0), kind(obj0) {} // TODO: remove this if possible.
  Type(Head h, Int l, Obj n, Obj k): head(h), len(l), name(n), kind(k) {}
} ALIGNED_TO_WORD;
DEF_SIZE(Type);


#define TYPE_LIST \
T(Type,             struct2, "name", t_Sym, "kind", t_Type_kind) \
T(Ptr,              prim) \
T(Int,              prim) \
T(Sym,              prim) \
T(Data,             prim) \
T(Env,              prim) \
T(Comment,          struct2, "is-expr", t_Bool, "val", t_Expr) \
T(Qua,              struct1, "expr", t_Expr) \
T(Unq,              struct1, "expr", t_Expr) \
T(Expand,           struct1, "exprs", t_Arr_Expr) \
T(Bang,             struct1, "expr", t_Expr) \
T(Quo,              struct1, "expr", t_Expr) \
T(Do,               struct1, "exprs", t_Arr_Expr) \
T(Scope,            struct1, "expr", t_Expr) \
T(Bind, struct4, "is-mut", t_Bool, "is-pub", t_Bool, "name", t_Sym, "expr", t_Expr) \
T(If,               struct3, "pred", t_Expr, "then", t_Expr, "else", t_Expr) \
T(Fn,               struct_fn) \
T(Call,             struct1, "exprs", t_Arr_Expr) \
T(Func,             struct_func) \
T(Accessor,         struct1, "name", t_Sym) \
T(Mutator,          struct1, "name", t_Sym) \
T(Splice,           struct1, "expr", t_Expr) \
T(Label,            struct3, "name", t_Expr, "type", t_Expr, "expr", t_Expr) \
T(Variad,           struct2, "name", t_Expr, "type", t_Expr) \
T(Src_loc,          struct_src_loc) \
T(Par,              struct3, "name", t_Sym, "type", t_Type, "dflt", t_Expr) \
T(Syn_struct,       struct1, "exprs", t_Arr_Expr) \
T(Syn_seq,          struct1, "exprs", t_Arr_Expr) \
T(Arr_Type,         arr, t_Type) \
T(Arr_Par,          arr, t_Par) \
T(Arr_Obj,          arr, t_Obj) \
T(Arr_Expr,         arr, t_Expr) \
T(Expr,             union_expr) \
T(Type_kind,        union_type_kind) \
T(Type_kind_unit,   unit) \
T(Type_kind_prim,   unit) \
T(Type_kind_arr,    struct1, "el-type", t_Type) \
T(Type_kind_struct, struct2, "fields", t_Arr_Par, "dispatcher", t_Dispatcher) \
T(Type_kind_union,  struct1, "variants", t_Arr_Type) \
T(Type_kind_class,  unit) \
T(Type_kind_var,    struct1, "name", t_Sym) \
T(Obj,              class_obj) \
T(Bool,             class_bool) \
T(Dispatcher,       class_dispatcher) \
T(File, struct4, "name", t_Data, "ptr", t_Ptr, "is-readable", t_Bool, "is_writeable", t_Bool) \


// type indices for the basic types allow us to dispatch on type using a single index value,
// rather than multiple pointer comparisons.
#define T(t, ...) ti_##t,
enum Type_index {
  TYPE_LIST
  ti_END
};
#undef T

// the basic types are prefixed with t_.
#define T(t, ...) Obj t_##t;
TYPE_LIST
#undef T


// in order for type indices to work, the known types must be allocated in a single slab.
static Type type_table[ti_END];


#define assert_valid_type_index(ti) assert((ti) >= 0 && (ti) < ti_END)

static Obj type_for_index(Int ti) {
  assert_valid_type_index(ti);
  return Obj(type_table + ti);
}


static Int type_index(Obj t) {
  // note: it is ok to return an invalid type index, to default in various switch dispatchers.
  Int ti = t.t - type_table;
  return Type_index(ti);
}


static Obj type_name(Obj t) {
  assert(t.is_type());
  Obj name = cmpd_el(t, 0);
  assert(name.is_sym());
  return name;
}


static Obj type_kind(Obj t) {
  assert(t.is_type());
  Obj kind = cmpd_el(t, 1);
  return kind;
}


static Bool is_kind_struct(Obj kind) {
  return kind.type() == t_Type_kind_struct;
}


static Bool is_kind_arr(Obj kind) {
  return kind.type() == t_Type_kind_arr;
}


UNUSED_FN static Obj kind_el_type(Obj kind) {
  assert(is_kind_arr(kind));
  return cmpd_el(kind, 0);
}


static Obj kind_fields(Obj kind) {
  assert(is_kind_struct(kind));
  return cmpd_el(kind, 0);
}


// flat array of unit type, singleton interleaved pairs.
static List global_singletons;


static Obj type_unit(Obj type) {
  // TODO: improve performance by using a hash table?
  for_ins(i, global_singletons.mem.len, 2) {
    if (global_singletons.mem.els[i] == type) {
      return global_singletons.mem.els[i + 1].ret();
    }
  }
  Obj s = cmpd_new_raw(type.ret(), 0);
  global_singletons.append(type.ret());
  global_singletons.append(s);
  return s.ret();
}


static Obj par_new(Chars n, Obj t) {
  return cmpd_new(t_Par.ret(), sym_new_from_chars(n), t.ret(), s_void.ret_val());
}


static Obj type_kind_init_unit() {
  return type_unit(t_Type_kind_unit);
}


static Obj type_kind_init_prim() {
  return type_unit(t_Type_kind_prim);
}


static Obj type_kind_init_arr(Obj el_type) {
  return cmpd_new(t_Type_kind_arr.ret(), el_type.ret());
}


static Obj type_kind_struct(Obj fields) {
  // owns fields.
  // all structs start with a nil dispatcher.
  return cmpd_new(t_Type_kind_struct.ret(), fields, s_nil.ret_val());
}


static Obj type_kind_init_struct1(Chars n0, Obj t0) {
  return type_kind_struct(cmpd_new(t_Arr_Par.ret(),
    par_new(n0, t0)));
}


static Obj type_kind_init_struct2(Chars n0, Obj t0, Chars n1, Obj t1) {
  return type_kind_struct(cmpd_new(t_Arr_Par.ret(),
    par_new(n0, t0), par_new(n1, t1)));
}


static Obj type_kind_init_struct3(Chars n0, Obj t0, Chars n1, Obj t1,
  Chars n2, Obj t2) {
  return type_kind_struct(cmpd_new(t_Arr_Par.ret(),
    par_new(n0, t0), par_new(n1, t1), par_new(n2, t2)));
}


static Obj type_kind_init_struct4(Chars n0, Obj t0, Chars n1, Obj t1,
  Chars n2, Obj t2, Chars n3, Obj t3) {
  return type_kind_struct(cmpd_new(t_Arr_Par.ret(),
    par_new(n0, t0), par_new(n1, t1), par_new(n2, t2), par_new(n3, t3)));
}


static Obj type_kind_init_struct_fn() {
  return type_kind_struct(cmpd_new(t_Type_kind_struct.ret(),
    par_new("is-native", t_Bool),
    par_new("is-macro", t_Bool),
    par_new("pars", t_Syn_seq),
    par_new("ret-type", t_Type),
    par_new("body", t_Expr)));
}


static Obj type_kind_init_struct_func() {
  return type_kind_struct(cmpd_new(t_Type_kind_struct.ret(),
    par_new("is-native", t_Bool),
    par_new("is-macro", t_Bool),
    par_new("env", t_Env),
    par_new("variad", t_Par),
    par_new("pars", t_Syn_seq),
    par_new("ret-type", t_Type),
    par_new("body", t_Expr)));
}


static Obj type_kind_init_struct_src_loc() {
  return type_kind_struct(cmpd_new(t_Type_kind_struct.ret(),
    par_new("path", t_Data),
    par_new("src", t_Data),
    par_new("pos", t_Int),
    par_new("len", t_Int),
    par_new("line", t_Int),
    par_new("col", t_Int)));
}


static Obj type_kind_init_union_expr() {
  return cmpd_new(t_Type_kind_struct.ret(),
      cmpd_new(t_Arr_Type.ret(),
      t_Int.ret(),
      t_Sym.ret(),
      t_Data.ret(),
      t_Bang.ret(),
      t_Quo.ret(),
      t_Do.ret(),
      t_Scope.ret(),
      t_Bind.ret(),
      t_If.ret(),
      t_Fn.ret(),
      t_Syn_struct.ret(),
      t_Syn_seq.ret(),
      t_Call.ret(),
      t_Expand.ret()));
}


static Obj type_kind_init_union_type_kind() {
  return cmpd_new(t_Type_kind_struct.ret(),
    cmpd_new(t_Arr_Type.ret(),
      t_Type_kind_unit.ret(),
      t_Type_kind_prim.ret(),
      t_Type_kind_arr.ret(),
      t_Type_kind_struct.ret(),
      t_Type_kind_union.ret(),
      t_Type_kind_class.ret(),
      t_Type_kind_var.ret()));
}


static Obj type_kind_init_class_obj() {
  return type_unit(t_Type_kind_class);
}


static Obj type_kind_init_class_bool() {
  return type_unit(t_Type_kind_class);
}


static Obj type_kind_init_class_dispatcher() {
  return type_unit(t_Type_kind_class);
}


static void type_add(Obj type, Chars c_name, Obj kind) {
  assert(type.rc());
  type.h->type = t_Type.ret();
  type.t->len = 2;
  type.t->name = sym_new_from_c(c_name);
  type.t->kind = kind;
}


static void type_init_table() {
  // the type table holds the core types, and is statically allocated,
  // which allows us to map between type pointers and Type_index indices.
  // even though the contents of the table are not yet initialized,
  // we can now set all of the core type t_<T> variables,
  // which are necessary for complete initialization of the types themselves.
  // TODO: improve rc semantics so that we do not have to manually retain each item.
#define T(t, ...) t_##t = Obj(type_table + ti_##t); t_##t.h->rc = (1<<1) + 1;
  TYPE_LIST
#undef T
}


static void obj_validate(Set* s, Obj o) {
  if (s->contains(o)) return;
  check(o.vld(), "invalid object: %p", o.r);
  if (o.is_val()) return;
  check(o.h->rc, "object rc == 0: %o", o);
  s->insert(o);
  obj_validate(s, o.type());
  if (!o.is_cmpd()) return;
  Int len = cmpd_len(o);
  if (o.type() == t_Type) {
    check(len == 2, "Type: bad len: %i", len);
  }
  for_in(i, cmpd_len(o)) {
    obj_validate(s, cmpd_el(o, i));
  }
}


static Obj type_init_values(Obj env) {
  // this must be called after sym_init, because this adds symbols for the core types.
  assert(global_sym_names.mem.len);
  #define T(t, k, ...) type_add(t_##t, #t, type_kind_init_##k(__VA_ARGS__));
  TYPE_LIST
  #undef T
  for_in(i, ti_END) {
    Obj o = type_for_index(i);
    env = env_bind(env, false, false, o.t->name.ret(), o.ret());
  }
  // validate type graph.
  Set s;
  for_in(i, ti_END) {
    Obj t = type_for_index(i);
    obj_validate(&s, t);
  }
  s.dealloc(false);
  return env;
}


#if OPTION_ALLOC_COUNT
static void type_cleanup() {
  global_singletons.mem.rel_dealloc();
  for_in(i, ti_END) {
    Obj o = type_for_index(i);
    o.t->kind.rel();
    o.t->kind = s_DISSOLVED; // not counted; further access is invalid.
  }
  // run final cleanup in reverse so that Type is cleaned up last;
  // the type slot is released first, and then rc_remove is called,
  // so that Type releases itself first and then verifies that its rc count is zero.
  for_in_rev(i, ti_END) {
    Obj o = type_for_index(i);
    o.h->type.rel();
    o.t->name.rel_val(); // order does not matter as long as name is always a symbol.
    check(o.h->rc == (1<<1) + 1, "type_cleanup: unexpected rc: %u: %o", o.h->rc, o);
  }
}
#endif

