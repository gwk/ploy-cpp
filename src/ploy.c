// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "25-eval.h"
#include "ploy-core.h"


static void parse_and_eval(Obj env, Obj path, Obj src, Array* sources, Bool out_val) {
  // owns path, src.
  BM e = NULL;
  Obj code = parse_src(data_SS(path), data_SS(src), &e);
  if (e) {
    err("parse error: ");
    errL(e);
    free(e);
    obj_rel(path);
    obj_rel(src);
    obj_rel(code);
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
      out_nl();
    }
    obj_rel(code);
    obj_rel(val);
  }
}


int main(int argc, BC argv[]) {
  
  assert_host_basic();
  assert(size_Obj == size_Word);
  set_process_name(argv[0]);
  sym_init();
  vol_err = VERBOSE;
  
  // parse arguments.
  BC paths[len_buffer];
  Int path_count = 0;
  BC expr = NULL;
  Bool out_val = false;
  for_imn(i, 1, argc) {
    BC arg = argv[i];
    errFLD("   %s", arg);
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
  Obj core_frame = env_frame_bind(obj_ret_val(CHAIN0), new_sym_from_BC("host-env"), obj_ret(host_env));
  Obj core_env = env_push(host_env, core_frame);
  
  Array sources = array0;
#if 1
  // run embedded core file.
  Obj path = new_data_from_BC("<core>");
  Obj src = new_data_from_BC(core_src);
  parse_and_eval(core_env, path, src, &sources, false);
  
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
#endif

#if OPT_ALLOC_COUNT
  obj_rel(env);
  mem_release_dealloc(global_sym_names.mem);
  mem_release_dealloc(sources.mem);
  counter_stats(vol_err);
#endif
  
  return 0;
}

