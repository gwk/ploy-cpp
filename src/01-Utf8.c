// Copyright 2011 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "00-basic.c"


Bool eq_Utf8(Utf8 a, Utf8 b) {
  return strcmp(a, b) == 0;
}


// get the base name of the path argument.
Utf8 path_base(Utf8 path) {
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
Int vol_err;

// the name of the current process.
Utf8 process_name;

// call in main to set process_name.
void set_process_name(Utf8 arg0) {
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

#define error(fmt, ...) { \
  err("%s error: %s:\n", (process_name ? process_name : __FILE__), __FUNCTION__); \
  err(fmt, ## __VA_ARGS__); \
  err("\n"); \
  exit(1); \
}

#define check(condition, fmt, ...) { if (!(condition)) error(fmt, ## __VA_ARGS__); }
