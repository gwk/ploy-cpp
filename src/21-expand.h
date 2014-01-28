// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "20-env.h"


static const BC trace_expand_prefix = "◇";      // white diamond
static const BC trace_post_expand_prefix = "▫"; // white small square


static Obj run_macro(Obj env, Obj macro, Int len, Obj* args) {
  return VOID;
}


static Obj expand_macro(Obj env, Int len, Obj* args) {
  return run_macro(env, args[0], len - 1, args + 1);
}


static Obj expand(Obj env, Obj code) {
  // owns code.
  if (!obj_is_vec(code)) {
    return code;
  }
  Int len = ref_len(code);
  Obj* els = vec_els(code);
  Obj hd = els[0];
  if (hd.u == QUO.u) {
    return code;
  }
  Obj expanded;
  if (hd.u == EXPA.u) {
      expanded = expand_macro(env, len - 1, els + 1);
  }
  else {
    // recursively expand vec.
    expanded = new_vec_raw(len);
    Obj* expanded_els = vec_els(expanded);
    for_in(i, len) {
      expanded_els[i] = expand(env, obj_retain_strong(els[i]));
    }
  }
  obj_release_strong(code);
  return expanded;
}

