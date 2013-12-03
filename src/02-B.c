static // Copyright 2011 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "01-basic.c"


// rather than cast between const and mutable bytes, use this union.
typedef union {
  BC c;
  BM m;
} B;


static Bool bc_eq(BC a, BC b) {
  return strcmp(a, b) == 0;
}


// get the base name of the path argument.
static BC path_base(BC path) {
  Int offset = 0;
  Int i = 0;
  loop {
    if (!path[i]) break; // EOS
    if (path[i] == '/') offset = i + 1;
    i += 1;
  }

  return path + offset; 
}


// errD prints if vol_err > 0.
static Int vol_err;

// the name of the current process.
static BC process_name;

// call in main to set process_name.
static void set_process_name(BC arg0) {
  process_name = path_base(arg0);
}


// stdio macros

#define out(fmt, ...) fprintf(stdout, fmt, ## __VA_ARGS__)
#define err(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)

#define outL(...) { out(__VA_ARGS__); fputs("\n", stdout); }
#define errL(...) { err(__VA_ARGS__); fputs("\n", stderr); }

#define errD(...)   { if (vol_err) err(__VA_ARGS__);  }
#define errLD(...)  { if (vol_err) errL(__VA_ARGS__); }


// error macros

#define warn(fmt, ...) errL("warning: " fmt, ## __VA_ARGS__)

#define error(fmt, ...) { \
  err("%s error: " fmt "\n", (process_name ? process_name : __FILE__), ## __VA_ARGS__); \
  exit(1); \
}

#define check(condition, fmt, ...) { if (!(condition)) error(fmt, ## __VA_ARGS__); }
