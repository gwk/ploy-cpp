// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "21-eval.h"


static void parse_and_eval(Obj env, Obj path, Obj src, Array* sources, Bool out_val) {
    BM e = NULL;
    Obj code = parse_src(path, src, &e);
    if (e) {
      err("parse error: ");
      errL(e);
      raw_dealloc(e);
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
      Obj val = eval(env, code);
      if (out_val && val.u != VOID.u) {
        write_repr_obj(stdout, val);
        out_nl();
      }
      obj_release_strong(val);
    }
    obj_release_strong(code);
}


int main(int argc, BC argv[]) {
  
  assert_host_basic();
  assert(size_Obj == size_Word);
  assert(int_val(new_int(-1)) == -1);
  set_process_name(argv[0]);
  sym_init();
  vol_err = VERBOSE;
  
  // parse arguments.
  BC paths[len_buffer];
  BC expr = NULL;
  Bool out_val = false;
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
      out_val = bc_eq(arg, "-e");
    }
    else {
      check(path_count < len_buffer, "exceeded max paths: %d", len_buffer);
      paths[path_count++] = arg;
    }
  }

  Obj host_frame = host_init();
  Obj global_env = env_cons(END, host_frame);
  
  Array sources = array0;
  
  // handle arguments.
  for_in(i, path_count) {
    Obj path = new_data_from_BC(paths[i]);
    Obj src = new_data_from_path(paths[i]);
    parse_and_eval(global_env, path, src, &sources, false);
  }
  if (expr) {
    Obj path = new_data_from_BC("-e ");
    Obj src = new_data_from_BC(expr);
    parse_and_eval(global_env, path, src, &sources, out_val);
  }
  obj_release_strong(global_env);
  
#if OPT_ALLOC_COUNT
  mem_release_dealloc(global_sym_names.mem);
  mem_release_dealloc(sources.mem);

#define CHECK_ALLOCS(group, name) \
  if (vol_err || total_allocs_##group != total_deallocs_##group) { \
    errFL("==== PLOY ALLOC STATS: " name ": alloc: %ld; dealloc: %ld; diff = %ld", \
     total_allocs_##group, total_deallocs_##group, total_allocs_##group - total_deallocs_##group); \
  }

#define CHECK_ALLOCS_REF(n) CHECK_ALLOCS(ref[st_##n], #n)
  
  CHECK_ALLOCS(raw, "raw")
  CHECK_ALLOCS(mem, "mem")
  CHECK_ALLOCS_REF(Data)
  CHECK_ALLOCS_REF(Vec)
  CHECK_ALLOCS_REF(I32)
  CHECK_ALLOCS_REF(I64)
  CHECK_ALLOCS_REF(U32)
  CHECK_ALLOCS_REF(U64)
  CHECK_ALLOCS_REF(F32)
  CHECK_ALLOCS_REF(F64)
  CHECK_ALLOCS_REF(File)
  CHECK_ALLOCS_REF(Func_host_1)
  CHECK_ALLOCS_REF(Func_host_2)
  CHECK_ALLOCS_REF(Func_host_3)
  CHECK_ALLOCS_REF(Reserved_A)
  CHECK_ALLOCS_REF(Reserved_B)
  CHECK_ALLOCS_REF(Reserved_C)
  CHECK_ALLOCS_REF(DEALLOC)

#endif // OPT_ALLOC_COUNT

  return 0;
}

