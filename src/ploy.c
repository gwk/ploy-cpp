// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "22-eval.h"


static void parse_and_eval(Obj env, Obj path, Obj src, Array* sources, Bool out_val) {
    BM e = NULL;
    Obj code = parse_src(obj_borrow(path), obj_borrow(src), &e);
    if (e) {
      err("parse error: ");
      errL(e);
      raw_dealloc(e);
      exit(1);
    }
    else {
      //obj_errL(code);
      array_append(sources, new_vec2(path, src));
      Obj val = eval_loop(env, obj_borrow(code));
      if (out_val) {
        write_repr_obj(stdout, val);
        out_nl();
      }
      obj_release(val);
    }
    obj_release(code);
}


int main(int argc, BC argv[]) {
  
  assert_host_basic();
  assert_obj_tags_are_valid();
  assert(size_Obj == size_Word);
  assert(int_val(new_int(-1)) == -1);
  set_process_name(argv[0]);
  sym_init();
  vol_err = VERBOSE;
  
  // parse arguments.
  BC paths[len_buffer];
  BC expr = NULL;
  Bool out_expr = false;
  Int path_count = 0;
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
      out_expr = bc_eq(arg, "-e");
    }
    else {
      check(path_count < len_buffer, "exceeded max paths: %d", len_buffer);
      paths[path_count++] = arg;
    }
  }

  Obj host_frame = host_init();
  Obj global_env = env_cons(host_frame, END);
  
  Array sources = array0;
  
  // handle arguments.
  for_in(i, path_count) {
    Obj path = new_data_from_BC(paths[i]);
    Obj src = new_data_from_path(paths[i]);
    parse_and_eval(obj_ref_borrow(global_env), path, src, &sources, false);
  }
  if (expr) {
    Obj path = new_data_from_BC("<expr>");
    Obj src = new_data_from_BC(expr);
    parse_and_eval(global_env, path, src, &sources, out_expr);
  }
  obj_release(global_env);
  
#if OPT_ALLOC_COUNT
  mem_release_dealloc(global_sym_names.mem);
  mem_release_dealloc(sources.mem);

#define CHECK_ALLOCS(group) \
  if (vol_err || total_allocs_##group != total_deallocs_##group) { \
    errFL("==== PLOY ALLOC STATS: " #group ": alloc: %ld; dealloc: %ld; diff = %ld", \
     total_allocs_##group, total_deallocs_##group, total_allocs_##group - total_deallocs_##group); \
  }

  CHECK_ALLOCS(raw)
  CHECK_ALLOCS(mem)
  CHECK_ALLOCS(ref)

#endif // OPT_ALLOC_COUNT

  return 0;
}

