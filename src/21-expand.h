// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "20-env.h"


static const BC trace_expand_prefix = "◇";      // white diamond
static const BC trace_post_expand_prefix = "▫"; // white small square


static Obj run_call_native(Obj env, Obj func, Int len, Obj* args, Bool is_macro);

static Obj expand_macro(Obj env, Int len, Obj* args) {
  check(len > 0, "empty macro expand");
  Obj macro_sym = args[0];
  check_obj(obj_is_sym(macro_sym), "expand argument 0 must be a Sym; found", macro_sym);
  Obj macro = env_get(env, macro_sym);
  return run_call_native(env, macro, len - 1, args + 1, true);
}


static Obj expand(Obj env, Obj code) {
  // owns code.
  if (!obj_is_vec_ref(code)) {
    return code;
  }
  Int len = vec_len(code);
  Obj* els = vec_els(code);
  Obj hd = els[0];
  if (hd.u == QUO.u) {
    return code;
  }
  if (hd.u == EXPA.u) {
      Obj expanded = expand_macro(env, len - 1, els + 1);
      obj_rel(code);
      return expand(env, expanded); // macro result may contain more expands; recursively expand.
  }
  else {
    // recursively expand vec.
    Obj expanded = new_vec_raw(len);
    Obj* expanded_els = vec_els(expanded);
    for_in(i, len) {
      expanded_els[i] = expand(env, obj_ret(els[i]));
    }
    obj_rel(code);
    return expanded;
  }
}

