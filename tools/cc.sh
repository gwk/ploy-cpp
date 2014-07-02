# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# c compiler wrapper.

# for options see http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options
# TODO: clang docs link?


cd $(dirname "$0")/..

cmd_prefix=''
if [[ "$1" == "-cmd" ]]; then
  shift
  cmd_prefix=echo # just print the entire complie command instead of actually running it.
fi

if [[ "$1" == "-dbg" ]]; then
  shift
  opts="\
-DDEBUG=1 \
-fstack-protector \
-fsanitize=undefined-trap \
-fsanitize-undefined-trap-on-error \
-fno-limit-debug-info \
"
#-fsanitize=local-bounds
else
  opts="\
-DDEBUG=0 \
-Ofast \
"
fi

if [[ "$#" == 0 ]]; then
  echo "no arguments; humans use make and scripts in sh/."
  exit 1
fi

set -e

mkdir -p _bld

$cmd_prefix clang \
-std=c11 \
-Werror \
-Weverything \
-Wno-overlength-strings \
-Wno-unused-parameter \
-Wno-gnu \
-Wno-vla \
-Wno-covered-switch-default \
-fstrict-aliasing \
-ftrapv \
-g \
-ferror-limit=4 \
$opts \
-I _bld \
"$@"


#-ftrapv                    Trap on integer overflow
#-ftrapv-handler=func-name  Specify the function to be called on overflow

#-fsanitize-memory-track-origins  Enable origins tracking in MemorySanitizer

#-fstrict-enums             Enable optimizations based on the strict definition of an enum's value range
#-fvectorize                Enable the loop vectorization passes
#-fslp-vectorize            Enable the superword-level parallelism vectorization passes
#-funroll-loops             Turn on loop unroller
#-freroll-loops             Turn on loop reroller

#-foptimize-sibling-calls   tail call elimination
#-mllvm -tailcallelim       tail calls?

#-freg-struct-return        Override the default ABI to return small structs in registers
