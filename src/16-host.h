// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "15-func-host.h"


static Obj host_write(Obj file, Obj data) {
  check_obj(ref_is_file(file), "write expected File object; found", file);
  check_obj(ref_is_data(data), "write expected Data object; found", data);

  return new_int(0);
}

