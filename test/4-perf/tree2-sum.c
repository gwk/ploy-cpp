// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include <stdio.h>
#include <stdlib.h>

typedef long Int;
typedef struct _Branch Branch;

typedef union {
  Int leaf; // leaf.
  Branch* branch; // branch.
} Node;

struct _Branch {
  Node els[2];
};


static const Int depth = 16;

Node gen_tree(Int n, Node t) {
  if (!n) {
    return t;
  } else {
    Branch* b = (Branch*)malloc(sizeof(Branch));
    b->els[0] = t;
    b->els[1] = t;
    return gen_tree(n - 1, (Node){.branch=b});
  }
}

Int sum_tree(Node t) {
  if (t.leaf & 1) { // tagged int.
    return t.leaf >> 1; // shift the tag bit off.
  } else {
    return sum_tree(t.branch->els[0]) + sum_tree(t.branch->els[1]);
  }
}

int main(int argc, char* argv[]) {
  Node leaf = {.leaf=((1<<1) | 1)};
  Node t = gen_tree(depth, leaf);
  Int sum = sum_tree(t);
  printf("%ld\n", sum);
  return 0;
}

