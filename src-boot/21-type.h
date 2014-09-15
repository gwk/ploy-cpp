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
T(Type, NULL) \
T(Ptr, NULL) \
T(Int, NULL) \
T(Sym, NULL) \
T(Data, NULL) \
T(Accessor, NULL) \
T(Mutator, NULL) \
T(Env, NULL) \
T(Unq, NULL) \
T(Qua, NULL) \
T(Expand, NULL) \
T(Comment, NULL) \
T(Eval, NULL) \
T(Quo, NULL) \
T(Do, NULL) \
T(Scope, NULL) \
T(Bind, NULL) \
T(If, NULL) \
T(Fn, NULL) \
T(Syn_struct_typed, "Syn-struct-typed") \
T(Syn_seq_typed, "Syn-seq-typed") \
T(Call, NULL) \
T(Func, NULL) \
T(File, NULL) \
T(Label, NULL) \
T(Variad, NULL) \
T(Src, NULL) \
T(Expr, NULL) \
T(Par, NULL) \
T(Syn_struct, "Syn-struct") \
T(Syn_seq, "Syn-seq") \
T(Mem_Obj, "Mem-Obj") \
T(Mem_Par, "Mem-Par") \
T(Mem_Expr, "Mem-Expr") \


// type indices for the basic types allow us to dispatch on type using a single index value,
// rather than multiple pointer comparisons.
#define T(t, n) ti_##t,
typedef enum {
  TYPE_LIST
  ti_END
} Type_index;
#undef T

// the basic types are prefixed with t_.
#define T(t, n) static Obj t_##t;
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


static void type_add(Obj type, Chars_const name_custom, Chars_const name_default) {
  Obj name = sym_new_from_chars(name_custom ?: name_default);
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
#define T(type, n) t_##type = rc_insert((Obj){.t=(type_table + ti_##type)});
  TYPE_LIST
#undef T
}


static Obj type_init_values(Obj env) {
  // this must be called after sym_init, because this adds symbols for the core types.
  assert(global_sym_names.mem.len);
  #define T(t, n) type_add(t_##t, n, #t);
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

