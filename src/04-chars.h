// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// null-terminated c string and file types.
// in general we ignore const correctness, casting const strings to Chars immediately.

#include "03-raw.h"


typedef Char* Chars;
typedef const Char* CharsC; // Chars-constant.


typedef FILE* CFile; // 'File' refers to the ploy type.
DEF_SIZE(CFile);


static Bool chars_eq(CharsC a, CharsC b) {
  return strcmp(a, b) == 0;
}


// get the base name of the path argument.
static CharsC charsC_path_base(CharsC path) {
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
  process_name = charsC_path_base(arg0);
}


// stderr utilities.

static void err(CharsC s) { fputs(s, stderr); }
static void err_nl() { err("\n"); }
static void err_flush() { fflush(stderr); }
static void errL(CharsC s) { err(s); err_nl(); }

#define errF(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#define errFL(fmt, ...) errF(fmt "\n", ## __VA_ARGS__)

// error macros

#define warn(fmt, ...) errL("warning: " fmt, ## __VA_ARGS__)

#define error(fmt, ...) { \
  errFL("%s error: " fmt, (process_name ? process_name : __FILE__), ## __VA_ARGS__); \
  exit(1); \
}

#define check(condition, fmt, ...) { if (!(condition)) error(fmt, ## __VA_ARGS__); }

