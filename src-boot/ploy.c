// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "28-eval.h"


static Obj parse_and_eval(Obj env, Obj path, Obj src, Array* sources, Bool out_val) {
  // owns path, src.
  Chars e = NULL;
  Obj code = parse_src(data_str(path), data_str(src), &e);
  if (e) {
    err("parse error: ");
    errL(e);
    raw_dealloc(e, ci_Chars);
    rc_rel(path);
    rc_rel(src);
    assert(code.u == obj0.u);
    exit(1);
  }
#if VERBOSE_PARSE
  errF("parse_and_eval: ");
  obj_err(path);
  err_nl();
  obj_errL(code);
#endif
  array_append(sources, struct_new2(rc_ret(s_Src), path, src));
  Step step = eval_struct(env, code);
  if (out_val && step.val.u != s_void.u) {
    write_repr(stdout, step.val);
    fputc('\n', stdout);
  }
  rc_rel(code);
  rc_rel(step.val);
  return step.env;
}


int main(int argc, Chars_const argv[]) {
  assert_host_basic();
  assert(size_Obj == size_Word);
  set_process_name(argv[0]);
  rc_init();
  sym_init();
  Int vol_err = VERBOSE;

  // parse arguments.
  Chars_const paths[len_buffer];
  Int path_count = 0;
  Chars_const expr = NULL;
  Bool should_output_val = false;
  Bool should_load_core = true;
  for_imn(i, 1, argc) {
    Chars_const arg = argv[i];
    if (chars_eq(arg, "-v")) { // verbose.
      vol_err = 1;
    } else if (chars_eq(arg, "-e") || chars_eq(arg, "-E")) { // expression.
      check(!expr, "multiple expression arguments");
      i++;
      check(i < argc, "missing expression argument");
      expr = argv[i];
      should_output_val = chars_eq(arg, "-e");
    } else if (chars_eq(arg, "-b")) { // bare (do not load core.ploy).
      should_load_core = false;
    } else {
      check(path_count < len_buffer, "exceeded max paths: %d", len_buffer);
      paths[path_count++] = arg;
    }
  }

  Obj env = rc_ret_val(s_ENV_END_MARKER);
  env = env_push_frame(env, data_new_from_chars(cast(Chars, "<host>")));
  env = host_init(env);

  // global array of (path, source) objects for error reporting.
  Array sources = array0;
  Obj path, src;

  if (should_load_core) { // run embedded core.ploy file.
    path = data_new_from_chars(cast(Chars, "<core>"));
    env = env_push_frame(env, rc_ret(path));
    src = data_new_from_chars(cast(Chars, core_src)); // TODO: breaks const correctness?
    env = parse_and_eval(env, path, src, &sources, false);
  }
  // handle arguments.
  for_in(i, path_count) {
    path = data_new_from_chars(cast(Chars, paths[i])); // TODO: breaks const correctness?
    Obj name = data_new_from_chars(cast(Chars, chars_path_base(paths[i]))); // TODO: breaks const correctness?
    env = env_push_frame(env, name);
    src = data_new_from_path(paths[i]);
    env = parse_and_eval(env, path, src, &sources, false);
  }
  if (expr) {
    path = data_new_from_chars(cast(Chars, "<expr>"));
    env = env_push_frame(env, rc_ret(path));
    src = data_new_from_chars(cast(Chars, expr)); // TODO: breaks const correctness?
    env = parse_and_eval(env, path, src, &sources, should_output_val);
  }

#if OPT_ALLOC_COUNT
  rc_rel(env);
  mem_release_dealloc(global_sym_names.mem);
  mem_release_dealloc(sources.mem);
  rc_cleanup();
  counter_stats(vol_err);
#endif

#if OPT_RC_TABLE_STATS
  rc_table_stats();
#endif

  return 0;
}
