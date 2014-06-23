# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

# note: opt prints callgraph info to stderr, and a dumb message to stdout.
# opt 3.4.1 outputs raw llvm symbols, some of which contain 0x01 bytes (ascii SOH).
# expose these with cat -v, which displays non-printing characters.

flag=$1
src=$2
dst=$3

opt -analyze "$1" "$2" >/dev/null 2>"$3".raw
cat -v "$3".raw > "$3"
