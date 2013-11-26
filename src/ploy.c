// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.


#include "02-obj.c"

int main(int argc, Utf8 argv[]) {
  
  assert_host_basic;
  assert(width_Int == 8); // 32 bit not yet supported.

  set_process_name(argv[0]);
  
  const Int max_paths = 1<<8;
  Utf8 paths[max_paths];
  Int path_count = 0;

  // parse arguments.
  for_imn(i, 1, argc) {
    Utf8 arg = argv[i];
    errLD("   %s", arg);
    if (eq_Utf8(arg, "-v")) {
      vol_err = 1;
    }
    else {
      check(path_count < max_paths, "exceeded max paths: %ld", max_paths);
      paths[path_count++] = arg;
    }
  }
  
  return 0;
}
