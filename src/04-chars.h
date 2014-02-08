// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// null-terminated c string types; CFile type.

#include "03-raw.h"


typedef Char* CharsM; // Chars-mutable.
typedef const Char* CharsC; // Chars-constant.

// currently unused.
typedef wchar_t* Utf32M;
typedef const wchar_t* Utf32C;

typedef FILE* CFile; // 'File' refers to the ploy type.


// rather than cast between const and mutable bytes, use this union type.
// this reduces the risk of casts that violate the c strict aliasing semantics.
typedef union {
  CharsC c;
  CharsM m;
} Chars;


static Bool bc_eq(CharsC a, CharsC b) {
  return strcmp(a, b) == 0;
}


// get the base name of the path argument.
static CharsC path_base(CharsC path) {
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
static CharsC process_name;

// call in main to set process_name.
static void set_process_name(CharsC arg0) {
  process_name = path_base(arg0);
}


// stdio utilities.

static void out(CharsC s) { fputs(s, stdout); }
static void err(CharsC s) { fputs(s, stderr); }

static void out_nl() { out("\n"); }
static void err_nl() { err("\n"); }

UNUSED_FN static void out_flush() { fflush(stdout); }
static void err_flush() { fflush(stderr); }

UNUSED_FN static void outL(CharsC s) { out(s); out_nl(); }
static void errL(CharsC s) { err(s); err_nl(); }

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

