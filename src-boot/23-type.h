// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// type objects.

#include "22-global.h"


struct _Type {
  Obj type;
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
T(Bind, struct4, "is-mut", t_Bool, "is-pub", t_Bool, "name", t_Sym, "expr", t_Expr) \
T(If,               struct3, "pred", t_Expr, "then", t_Expr, "else", t_Expr) \
T(Fn,               struct_fn) \
T(Syn_struct_typed, struct1, "exprs", t_Arr_Expr) \
T(Syn_seq_typed,    struct1, "exprs", t_Arr_Expr) \
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
T(Type_kind_union,  arr, t_Type) \
T(Type_kind_class,  unit) \
T(Type_kind_var,    struct1, "name", t_Sym) \
T(Obj,              class_obj) \
T(Bool,             class_bool) \
T(Dispatcher,       class_dispatcher) \
T(File, struct4, "name", t_Data, "ptr", t_Ptr, "is-readable", t_Bool, "is_writeable", t_Bool) \


// type indices for the basic types allow us to dispatch on type using a single index value,
// rather than multiple pointer comparisons.
#define T(t, ...) ti_##t,
typedef enum {
  TYPE_LIST
  ti_END
} Type_index;
#undef T

// the basic types are prefixed with t_.
#define T(t, ...) static Obj t_##t;
TYPE_LIST
#undef T


// in order for type indices to work, the known types must be allocated in a single slab.
static Type type_table[ti_END];


#define assert_valid_type_index(ti) assert((ti) >= 0 && (ti) < ti_END)

static Obj type_for_index(Type_index ti) {
  assert_valid_type_index(ti);
  return (Obj){.t=type_table + ti};
}


static Int type_index(Obj t) {
  Int ti = t.t - type_table;
  return cast(Type_index, ti);
}


static Obj type_name(Obj t) {
  assert(obj_is_type(t));
  Obj name = cmpd_el(t, 0);
  assert(obj_is_sym(name));
  return name;
}


static Obj type_kind(Obj t) {
  assert(obj_is_type(t));
  Obj kind = cmpd_el(t, 1);
  return kind;
}


static Bool is_kind_struct(Obj kind) {
  return is(obj_type(kind), t_Type_kind_struct);
}


static Bool is_kind_arr(Obj kind) {
  return is(obj_type(kind), t_Type_kind_arr);
}


// flat array of unit type, singleton interleaved pairs.
static Array global_singletons = array0;


static Obj type_unit(Obj type) {
  // TODO: improve performance by using a hash table?
  for_ins(i, global_singletons.mem.len, 2) {
    if (is(global_singletons.mem.els[i], type)) {
      return global_singletons.mem.els[i + 1];
    }
  }
  Obj s = cmpd_new_raw(rc_ret(type), 0);
  array_append(&global_singletons, rc_ret(type));
  array_append(&global_singletons, s);
  return rc_ret(s);
}


static Obj par_new(Chars_const n, Obj t) {
  return cmpd_new3(rc_ret(t_Par), sym_new_from_chars(n), rc_ret(t), rc_ret_val(s_void));
}


static Obj type_kind_init_unit() {
  return type_unit(t_Type_kind_unit);
}


static Obj type_kind_init_prim() {
  return type_unit(t_Type_kind_prim);
}


static Obj type_kind_init_arr(Obj el_type) {
  return cmpd_new1(rc_ret(t_Type_kind_arr), rc_ret(el_type));
}


static Obj type_kind_struct(Obj fields) {
  // owns fields.
  // all structs start with a nil dispatcher.
  return cmpd_new2(rc_ret(t_Type_kind_struct), fields, rc_ret_val(s_nil));
}


static Obj type_kind_init_struct1(Chars_const n0, Obj t0) {
  return type_kind_struct(cmpd_new1(rc_ret(t_Arr_Par),
    par_new(n0, t0)));
}


static Obj type_kind_init_struct2(Chars_const n0, Obj t0, Chars_const n1, Obj t1) {
  return type_kind_struct(cmpd_new2(rc_ret(t_Arr_Par),
    par_new(n0, t0), par_new(n1, t1)));
}


static Obj type_kind_init_struct3(Chars_const n0, Obj t0, Chars_const n1, Obj t1,
  Chars_const n2, Obj t2) {
  return type_kind_struct(cmpd_new3(rc_ret(t_Arr_Par),
    par_new(n0, t0), par_new(n1, t1), par_new(n2, t2)));
}


static Obj type_kind_init_struct4(Chars_const n0, Obj t0, Chars_const n1, Obj t1,
  Chars_const n2, Obj t2, Chars_const n3, Obj t3) {
  return type_kind_struct(cmpd_new4(rc_ret(t_Arr_Par),
    par_new(n0, t0), par_new(n1, t1), par_new(n2, t2), par_new(n3, t3)));
}


static Obj type_kind_init_struct_fn() {
  return type_kind_struct(cmpd_new5(rc_ret(t_Type_kind_struct),
    par_new("is-native", t_Bool),
    par_new("is-macro", t_Bool),
    par_new("pars", t_Syn_seq),
    par_new("ret-type", t_Type),
    par_new("body", t_Expr)));
}


static Obj type_kind_init_struct_func() {
  return type_kind_struct(cmpd_new7(rc_ret(t_Type_kind_struct),
    par_new("is-native", t_Bool),
    par_new("is-macro", t_Bool),
    par_new("env", t_Env),
    par_new("variad", t_Par),
    par_new("pars", t_Syn_seq),
    par_new("ret-type", t_Type),
    par_new("body", t_Expr)));
}


static Obj type_kind_init_struct_src_loc() {
  return type_kind_struct(cmpd_new6(rc_ret(t_Type_kind_struct),
    par_new("path", t_Data),
    par_new("src", t_Data),
    par_new("pos", t_Int),
    par_new("len", t_Int),
    par_new("line", t_Int),
    par_new("col", t_Int)));
}


static Obj type_kind_init_union_expr() {
  return cmpd_new14(rc_ret(t_Type_kind_struct),
    rc_ret(t_Int),
    rc_ret(t_Sym),
    rc_ret(t_Data),
    rc_ret(t_Bang),
    rc_ret(t_Quo),
    rc_ret(t_Do),
    rc_ret(t_Scope),
    rc_ret(t_Bind),
    rc_ret(t_If),
    rc_ret(t_Fn),
    rc_ret(t_Syn_struct_typed),
    rc_ret(t_Syn_seq_typed),
    rc_ret(t_Call),
    rc_ret(t_Expand));
}


static Obj type_kind_init_union_type_kind() {
  return cmpd_new7(rc_ret(t_Type_kind_struct),
    rc_ret(t_Type_kind_unit),
    rc_ret(t_Type_kind_prim),
    rc_ret(t_Type_kind_arr),
    rc_ret(t_Type_kind_struct),
    rc_ret(t_Type_kind_union),
    rc_ret(t_Type_kind_class),
    rc_ret(t_Type_kind_var));
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


static void type_add(Obj type, Chars_const c_name, Obj kind) {
  Obj name = sym_new_from_c(c_name);
  *type.t = (Type){
    .type=rc_ret(t_Type),
    .len=2,
    .name=name,
    .kind=kind
  };
}


static void type_init_table() {
  // the type table holds the core types, and is statically allocated,
  // which allows us to map between type pointers and Type_index indices.
  // even though the contents of the table are not yet initialized,
  // we can now set all of the core type t_<T> c pointer variables,
  // which are necessary for initialization of all other basic ploy objects.
  // we must also insert these objects into the rc table using the special rc_insert function.
#define T(type, ...) t_##type = rc_insert((Obj){.t=(type_table + ti_##type)});
  TYPE_LIST
#undef T
}


static Obj type_init_values(Obj env) {
  // this must be called after sym_init, because this adds symbols for the core types.
  assert(global_sym_names.mem.len);
  #define T(t, k, ...) type_add(t_##t, #t, type_kind_init_##k(__VA_ARGS__));
  TYPE_LIST
  #undef T
  for_in(i, ti_END) {
    Obj o = type_for_index((Type_index)i);
    env = env_bind(env, false, false, rc_ret(o.t->name), rc_ret(o));
  }
  return env;
}


#if OPTION_ALLOC_COUNT
static void type_cleanup() {
  mem_rel_dealloc(global_singletons.mem);
  for_in(i, ti_END) {
    Obj o = type_for_index(cast(Type_index, i));
    rc_rel(o.t->kind);
    o.t->kind = s_DISSOLVED; // not counted; further access is invalid.
  }
  // run final cleanup in reverse so that Type is cleaned up last;
  // the type slot is released first, and then rc_remove is called,
  // so that Type releases itself first and then verifies that its rc count is zero.
  for_in_rev(i, ti_END) {
    Obj o = type_for_index(cast(Type_index, i));
    rc_rel(o.t->type);
    rc_rel_val(o.t->name); // order does not matter as long as name is always a symbol.
    rc_remove(o);
  }
}
#endif

