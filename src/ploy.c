// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "28-eval.h"


static void parse_and_eval(Obj env, Obj path, Obj src, Array* sources, Bool out_val) {
  // owns path, src.
  Chars e = NULL;
  Obj code = parse_src(data_str(path), data_str(src), &e);
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
    array_append(sources, new_vec2(path, src));
    Obj val = eval_vec(env, code);
    if (out_val && val.u != VOID.u) {
      write_repr(stdout, val);
      fputc('\n', stdout);
    }
    obj_rel(code);
    obj_rel(val);
  }
}


int main(int argc, Chars_const argv[]) {

  assert_host_basic();
  assert(size_Obj == size_Word);
  set_process_name(argv[0]);
  sym_init();
  Int vol_err = VERBOSE;

  // parse arguments.
  Chars_const paths[len_buffer];
  Int path_count = 0;
  Chars_const expr = NULL;
  Bool should_output_val = false;
  for_imn(i, 1, argc) {
    Chars_const arg = argv[i];
    if (chars_eq(arg, "-v")) {
      vol_err = 1;
    }
    else if (chars_eq(arg, "-e") || chars_eq(arg, "-E")) {
      check(!expr, "multiple expression arguments");
      i++;
      check(i < argc, "missing expression argument");
      expr = argv[i];
      should_output_val = chars_eq(arg, "-e");
    }
    else {
      check(path_count < len_buffer, "exceeded max paths: %d", len_buffer);
      paths[path_count++] = arg;
    }
  }

  Obj host_frame = host_init();
  Obj host_name = new_data_from_chars(cast(Chars, "<host>"));
  Obj host_env = env_push(obj_ret_val(END), host_name, host_frame);

  // global array of (path, source) objects for error reporting.
  Array sources = array0;
  Obj path, src;

  // run embedded core file.
  path = new_data_from_chars(cast(Chars, "<core>"));
  Obj core_env = env_push(host_env, obj_ret(path), obj_ret_val(CHAIN0));
  src = new_data_from_chars(cast(Chars, core_src)); // TODO: breaks const correctness?
  parse_and_eval(core_env, path, src, &sources, false);

  // handle arguments.
  Obj env = core_env;

  for_in(i, path_count) {
    path = new_data_from_chars(cast(Chars, paths[i])); // TODO: breaks const correctness?
    Obj name = new_data_from_chars(cast(Chars, chars_path_base(paths[i]))); // TODO: breaks const correctness?
    env = env_push(env, name, obj_ret_val(CHAIN0));
    src = new_data_from_path(paths[i]);
    parse_and_eval(env, path, src, &sources, false);
  }
  if (expr) {
    path = new_data_from_chars(cast(Chars, "<expr>"));
    env = env_push(env, obj_ret(path), obj_ret_val(CHAIN0));
    src = new_data_from_chars(cast(Chars, expr)); // TODO: breaks const correctness?
    parse_and_eval(env, path, src, &sources, should_output_val);
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
