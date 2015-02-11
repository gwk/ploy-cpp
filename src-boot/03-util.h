// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

// miscellaneous utilities.

#include "02-std-types.h"


#define len_buffer Int(256) // standard buffer size for various Char arrays on the stack.


// stderr utilities: write info/error/debug messages to the console.

#define err(s) fputs((s), stderr)
#define err_nl() err("\n")
#define err_flush() fflush(stderr)
#define errL(s) { err(s); err_nl(); }

static void fmt_to_file(CFile f, Chars fmt);

template <typename T, typename... Ts>
static void fmt_to_file(CFile f, Chars fmt, T item, Ts... items);

// note: the final token pasting is between the comma and __VA_ARGS__.
// this is a gnu extension to elide the comma in the case where __VA_ARGS__ is empty.
#define errF(fmt, ...) fmt_to_file(stderr, fmt, ## __VA_ARGS__)
#define errFL(fmt, ...) errF(fmt "\n", ## __VA_ARGS__)


// error handling.

[[noreturn]] void fail(void);
[[noreturn]] void fail() {
  exit(1);
}

// write an error message and then exit.
#define error(fmt, ...) { \
  errFL("ploy error: " fmt, ## __VA_ARGS__); \
  fail(); \
}


// interpreter tracing.

struct Trace;

[[noreturn]] static void _exc_raise(Trace* trace);

// check that a condition is true; otherwise error.
#define check(condition, fmt, ...) { if (!(condition)) error(fmt, ## __VA_ARGS__); }

// NOTE: the exc macros expect env to be defined in the current scope.
#define exc_raise(fmt, ...) { fmt_to_file(stderr, fmt, ##__VA_ARGS__); _exc_raise(t); }
#define exc_check(condition, ...) if (!(condition)) exc_raise(__VA_ARGS__)


// iteration.

#define loop while (1) // infinite loop.

// for Int 'i' from 'm' to 'n' in increments of 's'.
#define for_imns(i, m, n, s) \
for (Int i = (m), _end_##i = (n), _step_##i = (s); i < _end_##i; i += _step_##i)

// produces the same values for i as above, but in reverse order.
#define for_imns_rev(i, m, n, s) \
for (Int i = (n) - 1, _end_##i = (m), _step_##i = (s); i >= _end_##i; i -= _step_##i)

// equivalent to for_imns(i, m, n, 1).
#define for_imn(i, m, n)      for_imns(i, (m), (n), 1)
#define for_imn_rev(i, m, n)  for_imns_rev(i, (m), (n), 1)

// equivalent to for_imns(i, 0, n, s).
#define for_ins(i, n, s)      for_imns(i, 0, (n), (s))
#define for_ins_rev(i, n, s)  for_imns_rev(i, 0, (n), (s))

// equivalent to for_imns(i, 0, n, 1).
#define for_in(i, n)      for_imns(i, 0, (n), 1)
#define for_in_rev(i, n)  for_imns_rev(i, 0, (n), 1)

// ranges.

template <typename T>
struct Range {
  T b;
  T e;
  Range(T _b, T _e): b(_b), e(_e) {}
  T begin() const { return b; }
  T end() const { return e; }
};


// slightly more concise wrappers around c++ range for.
#define for_val(el, coll) for (const auto el : coll)
#define for_ref(el, coll) for (const auto& el : coll)
#define for_mut(el, coll) for (auto& el : coll)


// misc macros.

// used to create switch statements that return strings for enum names.
#define CASE_RET_TOK(t) case t: return #t
#define CASE_RET_TOK_SPLIT(prefix, t) case prefix##t: return #t


// logic.

#define bit(x) (!!(x)) // 0 for falsy, 1 for truthy.
#define XOR(a, b) (bit(a) ^ bit(b)) // logical exclusive-or.


// integer functions.

// returns -1, 0, or 1 based on sign of input.
#define sign(x) ({__typeof__(x) __x = (x); __x > 0 ? 1 : (__x < 0 ? -1 : 0); })

static Int int_min(Int a, Int b) {
  return (a < b) ? a : b;
}

static Int int_max(Int a, Int b) {
  return (a > b) ? a : b;
}

static Int int_clamp(Int x, Int a, Int b) {
  return int_max(int_min(x, b), a);
}

UNUSED static Int int_pow2_fl(Int x) {
  if (x < 0) x *= -1;
  Int p = -1;
  while (x) {
    p++;
    x >>= 1;
  }
  return p;
}


// sanity checks.

static void assert_host_basic() {
  // a few sanity checks; called by main.
  assert(size_Raw == size_Int);
  assert(size_Raw == size_Uns);
  assert(size_Raw == size_Raw);
}
