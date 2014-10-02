Copyright 2014 George King.
Permission to use this file is granted in ploy/license.txt.


Ploy is an experimental programming language. It is somewhat similar to Lisp; the obvious differences are:
• The fundamental compound objects are typed structures and vectors, as opposed to cons pairs in Lisp;
• Macro expansion syntax is distinct from function call syntax ('<...>' and '(...)' respectively).
• Types are true objects that contain information describing the nature of their instances. 
• The runtime currently uses reference counting for memory management.

Ploy is currently in early development, and is released under the ISC license (similar to 2-clause BSD).

Contributions are welcome - there is a lot to do!

General Direction:
• Memory management via reference counting:
  • Partial/experimental support for collecting reference cycles via 'cycle coalescing'.
  • Weak references are not yet implemented.
• Ahead-of-time (AOT) compilation.
  • This implies no dynamic type or function creation.

Features that work:
• Tagged value/reference object implementation.
• Strong reference counting (leaks notwithstanding).
• basic macro expansion and code execution.
• a few core definitions.

Work to do:
• Semantics of nil/none and the Opt option/maybe type.
  • I think there should be some syntax dedicated to option checking.
  • Perhaps this syntax maybe generalized to first/rest variants of any union type.
• Semantics of void.
• Boolean semantics:
  • Should non-boolean types have implicit truthiness?
  • If so, should these be completely user-definable or are they constrained?
• Type semantics of special symbols like void, true, false.
• Type derivation.
• Argument splats?
• Start the compiler.

Here are some language design questions to explore:
• Does the distinct macro expansion syntax improve clarity? Does it reduce expressiveness?
• Does the simple syntax facilitate metaprogramming?
• Does the lack of traditional syntax (binary operators, dot accessors) hinder everyday programming?
• How well do various language features interact, in terms of semantics and implementation?
  • Reference counting, subject to reference cycle leaks.
  • constraints on mutability.
  • weak references.
  • closures, generators and coroutines.
  • compilation (specifically using LLVM).


------------
Requirements

Ploy is developed with the following toolchain:
• clang 3.5
• gnu bash 3.2.51
• gnu make 3.81
• python 3.4.1
• OS X 10.10

It has not been tested with gcc, but it should be! There might be a few clang-specific flags used in tools/cc.sh; these should not be necessary for correct builds.


-----
Usage

$ make
# build both _bld/ploy and _bld/ploy-dbg (the debug build with assertions).

$ sh/run.sh [src] [args...]
# make and run the debug build with malloc debug environment variables set.

$ make test
# run the tests.

Options:
  -e '<exprs>': evaluate expressions and output the final result to stdout.
  -t: trace evaluation: prints out verbose debug info for every evaluation step.
  -b: bare mode: do not load the core files; useful for debugging changes to the interpreter.
  -s: stats: log all stats on exit.

