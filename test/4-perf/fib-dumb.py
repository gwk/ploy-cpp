n = 28

def fib_dumb(n):
  if n == 0: return 0
  if n == 1: return 1
  return fib_dumb(n - 1) + fib_dumb(n - 2)

print(fib_dumb(n))
