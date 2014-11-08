
#include <stdio.h>

typedef long Int;

const Int n = 26;

Int fib_dumb(Int n) {
  if (n == 0) return 0;
  if (n == 1) return 1;
  return fib_dumb(n - 1) + fib_dumb(n - 2);
}

int main(int argc, char* argv[]) {
  printf("%ld\n", fib_dumb(n));
  return 0;
}

