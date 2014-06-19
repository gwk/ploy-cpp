# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

depth = 18

def gen_tree(n, t):
  if not n:
    return t
  else:
    return gen_tree(n - 1, (t, t))

def sum_tree(t):
  if isinstance(t, int):
    return t
  else:
    return sum_tree(t[0]) + sum_tree(t[1])

leaf = 1
t = gen_tree(depth, leaf)
print(sum_tree(t))
