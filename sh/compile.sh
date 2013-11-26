# Copyright 2011 George King.
# Permission to use this file is granted in ploy/license.txt.

# compiler and base options
# see http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options

root=$(dirname "$0")/..

clang \
-std=c11 \
-pedantic \
-Weverything \
-Wno-unused-parameter \
-Wno-unused-macros \
-Wno-missing-variable-declarations \
-Wno-missing-prototypes \
-Wno-vla \
-Wno-gnu \
-fstrict-aliasing \
-fstack-protector \
-fcatch-undefined-behavior \
-ftrapv \
-g \
"$@" \
"$root"/src/ploy.c
