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
T(Type) \


// sym indices.
#define T(t) ti_##t,
typedef enum {
  TYPE_LIST
  ti_END
} Type_index;
#undef T

#define T(t) static Obj t_##t;
TYPE_LIST
#undef T


static Type* type_table;


static Obj type_for_index(Type_index ti) {
  return (Obj){.t=type_table + ti};
}


static Obj type_add(Type_index ti, Chars_const name) {
  type_table[ti] = (Type){
    .type=obj0, // for ti == ti_Type, t_Type is not yet constructed.
    .len=2,
    .name=sym_new_from_chars(cast(Chars, name)),
    .kind=rc_ret_val(s_UNINIT)
  };
  Obj o = type_for_index(ti);
  rc_insert(o);
  // now that t_Type has been inserted into the table, we can use it as the metatype.
  o.t->type = rc_ret(type_for_index(ti_Type));
  return o;
}


static void type_init() {
  type_table = raw_alloc(ti_END * size_Type, ci_Type_table);
  #define T(t) t_##t = type_add(ti_##t, #t);
  TYPE_LIST
  #undef T
}


static Obj type_init_bindings(Obj env) {
  for_in(i, ti_END) {
    Obj o = type_for_index((Type_index)i);
    env = env_bind(env, rc_ret(o.t->name), rc_ret(o));
  }
  return env;
}


#if OPT_ALLOC_COUNT
static void type_cleanup() {
  for_in(i, ti_END) {
    Obj o = type_for_index((Type_index)i);
    rc_rel(o.t->type);
    rc_rel(o.t->name);
    rc_rel(o.t->kind);
    rc_remove(o);
  }
  raw_dealloc(type_table, ci_Type_table);
}
#endif

