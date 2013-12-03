// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "09-repr.c"


int main(int argc, BC argv[]) {
  
  assert_host_basic();
  assert(size_Obj == size_Word);

  set_process_name(argv[0]);
  init_syms();
  BC paths[len_buffer];
  Int path_count = 0;

  // parse arguments.
  for_imn(i, 1, argc) {
    BC arg = argv[i];
    errLD("   %s", arg);
    if (bc_eq(arg, "-v")) {
      vol_err = 1;
    }
    else {
      check(path_count < len_buffer, "exceeded max paths: %d", len_buffer);
      paths[path_count++] = arg;
    }
  }
  
  // handle arguments.
  for_in(i, path_count) {
    SS path = ss_from_BC(paths[i]);
    SS str = ss_from_path(paths[i]); // never freed; used for error reporting.
    BM e = NULL;
    Obj o = parse_ss(path, str, &e);
    if (e) {
      fputs(e, stderr);
      free(e);
    }
    //err_repr(o);
    obj_release(o);
  }
  return 0;
}
