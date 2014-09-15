# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# c compiler wrapper.

# for options see http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options
# TODO: clang docs link?


cd $(dirname "$0")/..

cmd_prefix=''
if [[ "$1" == "-cmd" ]]; then
  shift
  cmd_prefix=echo # just print the entire compile command instead of actually running it.
fi

if [[ "$1" == "-dbg" ]]; then
  shift
  opts="\
-DDEBUG=1 \
-fstack-protector \
-fsanitize=local-bounds \
-fsanitize=undefined \
-fno-limit-debug-info \
-O0
"
#-fsanitize=address # fails on clang 3.5: linker error for libclang_rt.asan_osx_dynamic.dylib.

else
  opts="\
-DDEBUG=0 \
-Ofast \
"
#-freg-struct-return        Override the default ABI to return small structs in registers
#-funroll-loops             Turn on loop unroller
#-freroll-loops             Turn on loop reroller
#-fvectorize                Enable the loop vectorization passes
#-fslp-vectorize            Enable the superword-level parallelism vectorization passes
#-fslp-vectorize-aggressive Enable the BB vectorization passes
#-fstrict-enums             Enable optimizations based on the strict definition of an enum's value range
fi

if [[ "$#" == 0 ]]; then
  echo "no arguments; humans use make and scripts in sh/."
  exit 1
fi

set -e

mkdir -p _bld

$cmd_prefix /usr/local/llvm/bin/clang \
-std=c11 \
-Werror \
-Weverything \
-Wno-overlength-strings \
-Wno-unused-parameter \
-Wno-gnu \
-Wno-covered-switch-default \
-Wno-class-varargs \
-fstrict-aliasing \
-ftrapv \
-g \
-ferror-limit=4 \
$opts \
-I _bld \
"$@"

#-fstandalone-debug      Emit full debug info for all types used by the program

#-fstack-protector-strong   Use a strong heuristic to apply stack protectors to functions
#-fstack-protector-all      Force the usage of stack protectors for all functions
#-fsanitize-memory-track-origins  Enable origins tracking in MemorySanitizer

#-ftrapv-handler=func-name  Specify the function to be called on overflow

#-foptimize-sibling-calls   tail call elimination
#-mllvm -tailcallelim       tail calls?

