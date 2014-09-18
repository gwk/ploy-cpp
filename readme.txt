Copyright 2014 George King.
Permission to use this file is granted in ploy/license.txt.


Ploy is an experimental programming language. It is most similar to Scheme; the obvious differences are:
• the fundamental compound objects are typed structures and vectors, as opposed to cons pairs in Scheme;
• macro expansion syntax is distinct from function call syntax ('<...>' and '(...)' respectively).
• types are true objects that contain information describing the nature of their instances. 
• the runtime currently uses reference counting for memory management; there is partial/experimental support for collecting reference cycles.


------------
Requirements

Ploy is developed with the following toolchain:
• clang 3.5
• gnu bash 3.2.51
• gnu make 3.81
• python 3.4.1
• OS X 10.9

It has not been tested with gcc, but it should be! There might be a few clang-specific flags used in tools/cc.sh; these may or may not be necessary for correct builds.


-----
Usage

$ make
# build both _bld/ploy and _bld/ploy-dbg (the debug build with assertions).

$ sh/run.sh [src] [args...]
# make and run the debug build with malloc debug environment variables set.

$ make test
# run the tests.

Options:
  -e <expr>: evaluate an expression and output the result to stdout.
  -t: trace evaluation: prints out verbose debug info for every evaluation step.
  -b: bare mode: do not load the core files; useful for debugging changes to the interpreter.
  -s: stats: log all stats on exit.

