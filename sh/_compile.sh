# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# for options see http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options
# TODO: clang docs link?


root=$(dirname "$0")/..
cd "$root"

if [[ "$1" == "-release" ]]; then
  shift
  defs="-DDEBUG=0 -Ofast"
else
  defs="-DDEBUG=1"
fi

if [[ "$#" == 0 ]]; then
  echo "no arguments; humans use build.sh and friends."
  exit 1
fi

if ! sh/is-product-current.sh build/text-to-c-literal sh/* src/text-to-c-literal.c; then
  echo "text-to-c-literal..."
  clang \
  -std=c11 \
  -Werror \
  -Weverything \
  src/text-to-c-literal.c \
  -o build/text-to-c-literal
fi

if ! sh/is-product-current.sh build/ploy-core.h sh/* src/text-to-c-literal.c src/ploy-core.h; then
  echo "ploy-core.h..."
  build/text-to-c-literal core_src < src/core.ploy > build/ploy-core.h
  echo "ploy..."
fi

# build ploy.
clang \
-std=c11 \
-Werror \
-Weverything \
-Wno-unused-macros \
-Wno-unused-function \
-Wno-unused-parameter \
-Wno-gnu \
-fstrict-aliasing \
-fstack-protector \
-fsanitize=undefined-trap \
-fsanitize-undefined-trap-on-error \
-g \
-ferror-limit=4 \
"$defs" \
-I build \
src/ploy.c \
"$@"


# -fbounds-checking         Enable run-time bounds checks
#-fno-limit-debug-info      Do not limit debug information produced to reduce size of debug binary
#-fsanitize-memory-track-origins  Enable origins tracking in MemorySanitizer
#-fsanatize=adress,undefined
#-ftrapv-handler=func-name  Specify the function to be called on overflow
#-ftrapv                    Trap on integer overflow
#-fwrapv                    Treat signed integer overflow as two's complement

#-mms-bitfields             Set the default structure layout to be compatible with the Microsoft compiler standard

#-fstrict-enums             Enable optimizations based on the strict definition of an enum's value range
#-fvectorize                Enable the loop vectorization passes
#-fslp-vectorize            Enable the superword-level parallelism vectorization passes
#-funroll-loops             Turn on loop unroller
#-freroll-loops             Turn on loop reroller

#-foptimize-sibling-calls   tail call elimination
#-mllvm -tailcallelim       tail calls?

# -fpcc-struct-return       Override the default ABI to return all structs on the stack
# -freg-struct-return       Override the default ABI to return small structs in registers
