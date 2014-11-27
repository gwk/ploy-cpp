// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// type objects.

#include "15-global.h"


struct Type {
  Head head;
  Int len;
  Obj name;
  Obj kind;
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
T(Bind,             struct3,  "is-pub", t_Bool, "name", t_Sym, "expr", t_Expr) \
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
T(Arr_Sym,          arr, t_Sym) \
T(Arr_Int,          arr, t_Int) \
T(Expr,             union_expr) \
T(Type_kind,        union_type_kind) \
T(Type_kind_unit,   unit) \
T(Type_kind_prim,   unit) \
T(Type_kind_arr,    struct1, "el-type", t_Type) \
T(Type_kind_struct, struct2, "fields", t_Arr_Par, "dispatcher", t_Dispatcher) \
T(Type_kind_union,  struct1, "variants", t_Arr_Type) \
T(Type_kind_class,  unit) \
T(Obj,              class_obj) \
T(Bool,             class_bool) \
T(Dispatcher,       class_dispatcher) \
T(File, struct4, "name", t_Data, "ptr", t_Ptr, "is-readable", t_Bool, "is_writeable", t_Bool) \


// the basic types are prefixed with t_.
#define T(t, ...) extern Obj t_##t;
TYPE_LIST
#undef T

#define T(t, ...) Obj t_##t = obj0;
TYPE_LIST
#undef T


static Obj type_name(Obj t) {
  assert(t.is_type());
  Obj name = t.cmpd_el(0);
  assert(name.is_sym());
  return name;
}


static Obj type_kind(Obj t) {
  assert(t.is_type());
  Obj kind = t.cmpd_el(1);
  return kind;
}


static Bool is_kind_struct(Obj kind)  { return kind.type() == t_Type_kind_struct; }
static Bool is_kind_arr(Obj kind)     { return kind.type() == t_Type_kind_arr; }
static Bool is_kind_unit(Obj kind)    { return kind.type() == t_Type_kind_unit; }


UNUSED static Obj kind_el_type(Obj kind) {
  assert(is_kind_arr(kind));
  return kind.cmpd_el(0);
}


static Obj kind_fields(Obj kind) {
  assert(is_kind_struct(kind));
  return kind.cmpd_el(0);
}


static Dict unit_inst_memo;

static Obj type_unit(Obj type) {
  Obj inst = unit_inst_memo.fetch(type);
  if (!inst.vld()) {
    inst = Obj::Cmpd_raw(type.ret(), 0);
    unit_inst_memo.insert(type.ret(), inst);
  }
  return inst.ret();
}


static Obj par_new(Chars n, Obj t) {
  return Obj::Cmpd(t_Par.ret(), Obj::Sym(n), t.ret(), s_void.ret_val());
}


static Obj type_kind_unit() {
  return type_unit(t_Type_kind_unit);
}


static Obj type_kind_prim() {
  return type_unit(t_Type_kind_prim);
}


static Obj type_kind_arr(Obj el_type) {
  return Obj::Cmpd(t_Type_kind_arr.ret(), el_type.ret());
}


static Obj type_kind_struct(Obj fields) {
  // owns fields.
  // all structs start with a nil dispatcher.
  return Obj::Cmpd(t_Type_kind_struct.ret(), fields, s_nil.ret_val());
}


static Obj type_kind_struct1(Chars n0, Obj t0) {
  return type_kind_struct(Obj::Cmpd(t_Arr_Par.ret(),
    par_new(n0, t0)));
}


static Obj type_kind_struct2(Chars n0, Obj t0, Chars n1, Obj t1) {
  return type_kind_struct(Obj::Cmpd(t_Arr_Par.ret(),
    par_new(n0, t0), par_new(n1, t1)));
}


static Obj type_kind_struct3(Chars n0, Obj t0, Chars n1, Obj t1,
  Chars n2, Obj t2) {
  return type_kind_struct(Obj::Cmpd(t_Arr_Par.ret(),
    par_new(n0, t0), par_new(n1, t1), par_new(n2, t2)));
}


static Obj type_kind_struct4(Chars n0, Obj t0, Chars n1, Obj t1,
  Chars n2, Obj t2, Chars n3, Obj t3) {
  return type_kind_struct(Obj::Cmpd(t_Arr_Par.ret(),
    par_new(n0, t0), par_new(n1, t1), par_new(n2, t2), par_new(n3, t3)));
}


static Obj type_kind_struct_fn() {
  return type_kind_struct(Obj::Cmpd(t_Type_kind_struct.ret(),
    par_new("is-native", t_Bool),
    par_new("is-macro", t_Bool),
    par_new("pars", t_Syn_seq),
    par_new("ret-type", t_Type),
    par_new("body", t_Expr)));
}


static Obj type_kind_struct_func() {
  return type_kind_struct(Obj::Cmpd(t_Type_kind_struct.ret(),
    par_new("is-native", t_Bool),
    par_new("is-macro", t_Bool),
    par_new("env", t_Env),
    par_new("ret-type", t_Type),
    par_new("variad", t_Par),
    par_new("assoc", t_Par),
    par_new("pars", t_Syn_seq),
    par_new("body", t_Expr)));
}


static Obj type_kind_struct_src_loc() {
  return type_kind_struct(Obj::Cmpd(t_Type_kind_struct.ret(),
    par_new("path", t_Data),
    par_new("src", t_Data),
    par_new("pos", t_Int),
    par_new("len", t_Int),
    par_new("line", t_Int),
    par_new("col", t_Int)));
}


static Obj type_kind_union_expr() {
  return Obj::Cmpd(t_Type_kind_struct.ret(),
      Obj::Cmpd(t_Arr_Type.ret(),
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


static Obj type_kind_union_type_kind() {
  return Obj::Cmpd(t_Type_kind_struct.ret(),
    Obj::Cmpd(t_Arr_Type.ret(),
      t_Type_kind_unit.ret(),
      t_Type_kind_prim.ret(),
      t_Type_kind_arr.ret(),
      t_Type_kind_struct.ret(),
      t_Type_kind_union.ret(),
      t_Type_kind_class.ret()));
}


static Obj type_kind_class_obj() {
  return type_unit(t_Type_kind_class);
}


static Obj type_kind_class_bool() {
  return type_unit(t_Type_kind_class);
}


static Obj type_kind_class_dispatcher() {
  return type_unit(t_Type_kind_class);
}


static Obj derive_arr_type_sym(Obj name, Obj el_type) {
  assert(name.is_sym());
  assert(el_type.is_type());
  String s("(");
  s.append(name.sym_data().data_str());
  s.append(" -E=");
  s.append(type_name(el_type).sym_data().data_str());
  s.push_back(')');
  return Obj::Sym(Str(s), false);
}


static Dict arr_types_memo;

static Obj arr_type(Obj el_type) {
  Obj type = arr_types_memo.fetch(el_type);
  if (!type.vld()) {
    Obj name = derive_arr_type_sym(s_Arr, el_type);
    Obj kind = type_kind_arr(el_type.ret());
    type = Obj::Cmpd(t_Type.ret(), name, kind);
    arr_types_memo.insert(el_type.ret(), type);
    //global_push(type); // TODO: necessary?
  }
  return type.ret();
}


static Dict labeled_args_types_memo;

static Obj labeled_args_type(Obj el_type) {
  Obj type = labeled_args_types_memo.fetch(el_type);
  if (!type.vld()) {
    Obj name = derive_arr_type_sym(s_Labeled_args, el_type);
    Obj kind = type_kind_struct2("names", t_Arr_Sym, "args", arr_type(el_type));
    type = Obj::Cmpd(t_Type.ret(), name, kind);
    labeled_args_types_memo.insert(el_type.ret(), type);
    //global_push(type); // TODO: necessary?
  }
  return type.ret();
}


static void fix_arr_type(Obj el_type, Obj type) {
  Obj name = derive_arr_type_sym(s_Arr, el_type);
  type.cmpd_el_move(0).rel_val();
  type.cmpd_put(0, name);
  arr_types_memo.insert(el_type.ret(), type.ret());
}


static void type_init_vars() {
  // this must happen before any other objects are created,
  // so that the t_* values are not obj0.
  // thus we must initialize the types in two phases.
  // note that Cmpd_raw does not retain the type argument.
#define T(t, ...) t_##t = Obj::Cmpd_raw(t_Type, 2);
  TYPE_LIST
#undef T
  // t_Type does not yet point to itself.
  assert(t_Type.h->type == null);
  t_Type.h->type = t_Type.r;
}


static Obj type_complete(Obj env, Obj type, Chars c_name, Obj kind) {
  type.type().ret(); // now that Type points to itself, each instance can retain its type.
  type.t->name = Obj::Sym_from_c(c_name);
  type.t->kind = kind;
  global_push(type);
  type.rel(); // now that the types are in the global list, release them.
  return env_bind(env, false, type.t->name.ret(), type.ret());
}


static void obj_validate(Set* s, Obj o) {
  if (s->contains(o)) return;
  check(o.vld(), "invalid object: %p", o.r);
  if (o.is_val()) return;
  check(o.h->rc, "object rc == 0: %o", o);
  s->insert(o);
  obj_validate(s, o.type());
  if (!o.is_cmpd()) return;
  Int len = o.cmpd_len();
  if (o.type() == t_Type) {
    check(len == 2, "Type: bad len: %i", len);
  }
  for_in(i, o.cmpd_len()) {
    obj_validate(s, o.cmpd_el(i));
  }
}


static Obj type_init_values(Obj env) {
  // this must be called after sym_init, because this adds symbols for the core types.
  assert(sym_names.len());
  #define T(t, k, ...) env = type_complete(env, t_##t, #t, type_kind_##k(__VA_ARGS__));
  TYPE_LIST
  #undef T
  fix_arr_type(t_Type, t_Arr_Type);
  fix_arr_type(t_Par, t_Arr_Par);
  fix_arr_type(t_Obj, t_Arr_Obj);
  fix_arr_type(t_Expr, t_Arr_Expr);
  fix_arr_type(t_Sym, t_Arr_Sym);
  fix_arr_type(t_Int, t_Arr_Int);
  // validate type graph.
  Set s;
  #define T(t, k, ...) obj_validate(&s, t_##t);
  TYPE_LIST
  #undef T
  s.dealloc(false);
  return env;
}


#if OPTION_ALLOC_COUNT
static void type_cleanup() {
  unit_inst_memo.rel_els_dealloc();
  arr_types_memo.rel_els_dealloc();
  labeled_args_types_memo.rel_els_dealloc();
  Uns type_rc = t_Type.rc();
  if (type_rc != 1) { // only referent to Type should be itself.
    errFL("LEAK: Type rc = %u", type_rc);
  }
  // there is no easy way for type to release itself, so instead just decrement the counts.
  Obj* els = t_Type.cmpd_els_unchecked();
  els[0].rel_val();
  els[1].rel_val();
  counter_dec(ci_Cmpd_rc);
  counter_dec(ci_Cmpd_alloc);
}
#endif
