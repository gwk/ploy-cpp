// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "32-eval.h"


static Obj parse_and_eval(Dict* src_locs, Obj env, Obj path, Obj src, Bool should_output_val) {
  // owns path, src.
  Chars e = NULL;
  Obj code = parse_src(src_locs, path, src, &e);
  rc_rel(path);
  rc_rel(src);
  if (e) {
    err("parse error: ");
    errL(e);
    raw_dealloc(e, ci_Chars);
    assert(is(code, obj0));
    fail();
  }
#if VERBOSE_PARSE
  errFL("parse_and_eval: %o\n%o", path, code);
#endif
  Step step = eval_mem_expr(env, code);
  if (should_output_val && !is(step.res.val, s_void)) {
    write_repr(stdout, step.res.val);
    fputc('\n', stdout);
  }
  rc_rel(code);
  rc_rel(step.res.val);
  return step.res.env;
}


int main(int argc, Chars_const argv[]) {
  assert_host_basic();
  assert(size_Obj == size_Word);
  rc_init();
  type_init_table();
  sym_init(); // requires type_init_table.
  env_init();
  Obj env = type_init_values(rc_ret(s_ENV_END)); // requires sym_init.
  env = host_init(env);

  // parse arguments.
  Chars_const paths[len_buffer];
  Int path_count = 0;
  Chars_const expr = NULL;
  Bool should_output_val = false;
  Bool should_load_core = true;
  Bool should_log_stats = false;
  for_imn(i, 1, argc) {
    Chars_const arg = argv[i];
    check(arg[0], "empty argument");
    if (chars_eq(arg, "-t")) { // verbose eval.
      trace_eval = true;
    } else if (chars_eq(arg, "-s")) { // stats.
      should_log_stats = true;
    } else if (chars_eq(arg, "-e") || chars_eq(arg, "-E")) { // expression.
      check(!expr, "multiple expression arguments");
      i++;
      check(i < argc, "missing expression argument");
      expr = argv[i];
      should_output_val = chars_eq(arg, "-e");
    } else if (chars_eq(arg, "-b")) { // bare (do not load core.ploy).
      should_load_core = false;
    } else {
      check(arg[0] != '-', "unknown option: %s", arg);
      check(path_count < len_buffer, "exceeded max paths: %d", len_buffer);
      paths[path_count++] = arg;
    }
  }

  Dict src_locs = dict0;
  Obj path, src;

  if (should_load_core) { // run embedded core.ploy file.
    env = env_push_frame(env);
    path = data_new_from_chars("<core>");
    src = data_new_from_chars(core_src);
    env = parse_and_eval(&src_locs, env, path, src, false);
  }
  // handle arguments.
  for_in(i, path_count) {
    env = env_push_frame(env);
    path = data_new_from_chars(paths[i]);
    src = data_new_from_path(paths[i]);
    env = parse_and_eval(&src_locs, env, path, src, false);
  }
  if (expr) {
    env = env_push_frame(env);
    path = data_new_from_chars("<expr>");
    src = data_new_from_chars(expr);
    env = parse_and_eval(&src_locs, env, path, src, should_output_val);
  }

#if OPTION_ALLOC_COUNT
  // cleanup in reverse order.
  dict_rel(&src_locs);
  dict_dealloc(&src_locs);
  global_cleanup();
  rc_rel(env);
  env_cleanup();
  // release but do not clear to facilitate debugging during type_cleanup.
  mem_rel_no_clear(global_sym_names.mem);
  type_cleanup();
  rc_cleanup();
  mem_dealloc_no_clear(global_sym_names.mem);
  counter_stats(should_log_stats);
#endif

#if OPTION_RC_TABLE_STATS
  rc_table_stats();
#endif

  return 0;
}
