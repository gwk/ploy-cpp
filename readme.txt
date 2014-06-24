Copyright 2014 George King.
Permission to use this file is granted in ploy/license.txt.


Ploy is an experimental programming language. It is most similar to Scheme; the obvious differences are:
• the fundamental container objects are vectors of arbitrary length, as opposed to cons pairs;
• macro expansion syntax is distinct from function call syntax ('<>' and '()' respectively).
• there is syntax for 'fat chains': linked lists where each cell has more than one head element.
• the language currently does not support object mutation of any kind; eventually it will have allow certain restricted mutations. this means that it is not yet possible to define mutually recursive functions.
• the runtime currently uses reference counting for memory management; I may eventually add a mechanism to deal with reference cycles.

Currently the language has a very simple, fixed set of types; my goal is to add a more elaborate type system into the core of the language.


Requirements:

Ploy is developed with the following toolchain:
• clang 3.4.1
• gnu bash 3.2.51
• gnu make 3.81
• python 3.4.1

It has not been tested with gcc, but it should be! There might be a few clang-specific flags used in tools/cc.sh; these may or may not be necessary for correct builds.


Usage:

$ make test

This will build both _bld/ploy and _bld/ploy-dbg (the debug version with assertions). The interpreter binaries can then be invoked as follows:

$ _bld/ploy <source.ploy ...>


Options:
  -v: verbose mode: prints out runtime information.
  -e <expr>: evaluate an expression and output the result to stdout.

Vectors and linked lists:
• vector: [0 1 2 3]
• empty vector: []
• chain (linked list): [:0 1 2 3] or [|0|1|2|3]
• empty chain: [:]
• fat chain of pairs: [|0 0|1 -1|2 -2|3 -3]
• fat chain with variable width of cells: [||1|2 2|3 3 3] # note the zero-width cell at the head.
• fat chain containing a single zero-width cell: [|] # not the same as [:]!

