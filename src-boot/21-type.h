// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

// type objects.

#include "20-struct.h"


struct _Type {
  Obj type;
  Int len;
  Obj name;
  Obj kind;
} ALIGNED_TO_WORD;
DEF_SIZE(Type);


#define TYPE_LIST \
T(Type,             struct) \
T(Ptr,              prim) \
T(Int,              prim) \
T(Sym,              prim) \
T(Data,             prim) \
T(Accessor,         struct) \
T(Mutator,          struct) \
T(Env,              prim) \
T(Unq,              struct) \
T(Qua,              struct) \
T(Expand,           mem) \
T(Comment,          struct) \
T(Eval,             struct) \
T(Quo,              struct) \
T(Do,               mem) \
T(Scope,            struct) \
T(Bind,             struct) \
T(If,               struct) \
T(Fn,               struct) \
T(Syn_struct_typed, mem) \
T(Syn_seq_typed,    mem) \
T(Call,             mem) \
T(Func,             struct) \
T(File,             struct) \
T(Label,            struct) \
T(Variad,           struct) \
T(Src,              struct) \
T(Expr,             union) \
T(Par,              struct) \
T(Syn_struct,       mem) \
T(Syn_seq,          mem) \
T(Mem_Obj,          mem) \
T(Mem_Par,          mem) \
T(Mem_Expr,         mem) \
T(Type_kind,        union) \
T(Type_kind_unit,   prim) \
T(Type_kind_prim,   prim) \
T(Type_kind_mem,    struct) \
T(Type_kind_struct, struct) \
T(Type_kind_union,  struct) \
T(Type_kind_class,  struct) \
T(Type_kind_var,    struct) \


// type indices for the basic types allow us to dispatch on type using a single index value,
// rather than multiple pointer comparisons.
#define T(t, k) ti_##t,
typedef enum {
  TYPE_LIST
  ti_END
} Type_index;
#undef T

// the basic types are prefixed with t_.
#define T(t, k) static Obj t_##t;
TYPE_LIST
#undef T


// in order for type indices to work, the known types must be allocated in a single slab.
static Type* type_table;


#define assert_valid_type_index(ti) assert((ti) >= 0 && (ti) < ti_END)

static Obj type_for_index(Type_index ti) {
  assert_valid_type_index(ti);
  return (Obj){.t=type_table + ti};
}


static Type_index type_index(Obj t) {
  Int ti = t.t - type_table;
  assert_valid_type_index(ti);
  return cast(Type_index, ti);
}


static Obj type_name(Obj t) {
  Obj name = struct_el(t, 0);
  assert(obj_is_sym(name));
  return name;
}


static void type_add(Obj type, Chars_const c_name) {
  Obj name = sym_new_from_c(c_name);
  *type.t = (Type){
    .type=rc_ret(t_Type),
    .len=2,
    .name=name,
    .kind=rc_ret_val(s_UNINIT)
  };
}


static void type_init_table() {
  // the first step is to create the table, a single memory slab of the basic types.
  // this allows us to map between type pointers and Type_index indices.
  type_table = raw_alloc(ti_END * size_Type, ci_Type_table);
  // even though the contents of the table are not yet initialized,
  // we can now set all of the core type t_<T> c pointer variables,
  // which are necessary for initialization of all other basic ploy objects.
  // we must also insert these objects into the rc table using the special rc_insert function.
#define T(type, k) t_##type = rc_insert((Obj){.t=(type_table + ti_##type)});
  TYPE_LIST
#undef T
}


static Obj type_init_values(Obj env) {
  // this must be called after sym_init, because this adds symbols for the core types.
  assert(global_sym_names.mem.len);
  #define T(t, k) type_add(t_##t, #t);
  TYPE_LIST
  #undef T
  for_in(i, ti_END) {
    Obj o = type_for_index((Type_index)i);
    env = env_bind(env, false, rc_ret(o.t->name), rc_ret(o));
  }
  return env;
}


#if OPT_ALLOC_COUNT
static void type_cleanup() {
  // run cleanup in reverse so that Type is cleaned up last.
  // the type slot is released first, and then rc_remove is called,
  // so that Type releases itself first and then verifies that its rc count is zero.  
  for_in_rev(i, ti_END) {
    Obj o = type_for_index(cast(Type_index, i));
    rc_rel(o.t->type);
    rc_rel(o.t->name);
    rc_rel(o.t->kind);
    rc_remove(o);
  }
  raw_dealloc(type_table, ci_Type_table);
}
#endif

