// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "25-eval.h"


static void parse_and_eval(Obj env, Obj path, Obj src, Array* sources, Bool out_val) {
  // owns path, src.
  CharsM e = NULL;
  Obj code = parse_src(data_SS(path), data_SS(src), &e);
  if (e) {
    err("parse error: ");
    errL(e);
    raw_dealloc(e, ci_Chars);
    obj_rel(path);
    obj_rel(src);
    assert(code.u == obj0.u);
    exit(1);
  }
  else {
#if VERBOSE_PARSE
    errF("parse_and_eval: ");
    obj_err(path);
    err_nl();
    obj_errL(code);
#endif
    array_append_move(sources, new_vec2(path, src));
    Obj val = eval_vec(env, code);
    if (out_val && val.u != VOID.u) {
      write_repr(stdout, val);
      fputc('\n', stdout);
    }
    obj_rel(code);
    obj_rel(val);
  }
}


int main(int argc, CharsC argv[]) {
  
  assert_host_basic();
  assert(size_Obj == size_Word);
  set_process_name(argv[0]);
  sym_init();
  Int vol_err = VERBOSE;
  
  // parse arguments.
  CharsC paths[len_buffer];
  Int path_count = 0;
  CharsC expr = NULL;
  Bool out_val = false;
  for_imn(i, 1, argc) {
    CharsC arg = argv[i];
    if (bc_eq(arg, "-v")) {
      vol_err = 1;
    }
    else if (bc_eq(arg, "-e") || bc_eq(arg, "-E")) {
      check(!expr, "multiple expression arguments");
      i++;
      check(i < argc, "missing expression argument");
      expr = argv[i];
      out_val = bc_eq(arg, "-e");
    }
    else {
      check(path_count < len_buffer, "exceeded max paths: %d", len_buffer);
      paths[path_count++] = arg;
    }
  }
  
  Obj host_frame = host_init();
  Obj host_env = env_push(obj_ret_val(CHAIN0), host_frame);
  Obj core_env = env_push(host_env, obj_ret_val(CHAIN0));
  
  Array sources = array0;
  Obj path, src;
#if 1
  // run embedded core file.
  path = new_data_from_BC("<core>");
  src = new_data_from_BC(core_src);
  parse_and_eval(core_env, path, src, &sources, false);
#endif
  Obj env = env_push(core_env, obj_ret_val(CHAIN0));

  // handle arguments.
  for_in(i, path_count) {
    path = new_data_from_BC(paths[i]);
    src = new_data_from_path(paths[i]);
    parse_and_eval(env, path, src, &sources, false);
  }
  if (expr) {
    path = new_data_from_BC("<cmd>");
    src = new_data_from_BC(expr);
    parse_and_eval(env, path, src, &sources, out_val);
  }

#if OPT_ALLOC_COUNT
  obj_rel(env);
  mem_release_dealloc(global_sym_names.mem);
  mem_release_dealloc(sources.mem);
  if (vol_err) {
    counter_stats(vol_err);
  }
#endif
  
  return 0;
}

