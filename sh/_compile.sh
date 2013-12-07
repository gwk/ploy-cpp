# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# for options see http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options
# TODO: clang docs link?


root=$(dirname "$0")/..

if [[ "$1" == "-release" ]]; then
  shift
  release_defs="-DNDEBUG=1"
fi

if [[ "$#" == 0 ]]; then
  echo "no arguments; humans use build.sh and friends."
  exit 1
fi

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
-fcatch-undefined-behavior \
-ftrapv \
-g \
"$release_defs" \
"$root"/ploy.c \
"$@"
