// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "01-word.h"


typedef Char* BM; // Bytes-mutable.
typedef const Char* BC; // Bytes-constant.

typedef wchar_t* Utf32M;
typedef const wchar_t* Utf32C;

typedef FILE* File;


// rather than cast between const and mutable bytes, use this union.
typedef union {
  BC c;
  BM m;
} B; // Bytes


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


// the name of the current process.
static BC process_name;

// call in main to set process_name.
static void set_process_name(BC arg0) {
  process_name = path_base(arg0);
}


// stdio utilities.

static void out(BC s) { fputs(s, stdout); }
static void err(BC s) { fputs(s, stderr); }

static void out_nl() { out("\n"); }
static void err_nl() { err("\n"); }

static void out_flush() { fflush(stdout); }
static void err_flush() { fflush(stderr); }

static void outL(BC s) { out(s); out_nl(); }
static void errL(BC s) { err(s); err_nl(); }

#define outF(fmt, ...) fprintf(stdout, fmt, ## __VA_ARGS__)
#define errF(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)

#define outFL(fmt, ...) outF(fmt "\n", ## __VA_ARGS__)
#define errFL(fmt, ...) errF(fmt "\n", ## __VA_ARGS__)

// errD prints if vol_err > 0.
static Int vol_err;

#ifdef NDEBUG
#define errD(...);
#define errLD(...);
#define errFD(...);
#define errFLD(...);
#else
#define errD(...)   { if (vol_err) err(__VA_ARGS__);  }
#define errLD(...)  { if (vol_err) errL(__VA_ARGS__); }
#define errFD(...)  { if (vol_err) errF(__VA_ARGS__); }
#define errFLD(...) { if (vol_err) errFL(__VA_ARGS__); }
#endif

// error macros

#define warn(fmt, ...) errL("warning: " fmt, ## __VA_ARGS__)

#define error(fmt, ...) { \
  errFL("%s error: " fmt, (process_name ? process_name : __FILE__), ## __VA_ARGS__); \
  exit(1); \
}

#define check(condition, fmt, ...) { if (!(condition)) error(fmt, ## __VA_ARGS__); }

