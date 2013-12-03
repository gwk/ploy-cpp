# Copyright 2011 George King.
# Permission to use this file is granted in ploy/license.txt.

# compiler and base options
# see http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options

root=$(dirname "$0")/..

if [[ "$#" == 0 ]]; then
  echo "no arguments; humans use build.sh or parse.sh."
  exit 1
fi

clang \
-std=c11 \
-Werror \
-Weverything \
-Wno-unused-function \
-Wno-unused-parameter \
-Wno-gnu \
-fstrict-aliasing \
-fstack-protector \
-fcatch-undefined-behavior \
-ftrapv \
-g \
"$@" \
"$root"/src/ploy.c
