# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

# note: opt prints the callgraph to stderr, and a dumb message to stdout.
# opt 3.4.1 outputs raw llvm symbols, some of which contain 0x01 bytes (ascii SOH).
# expose these with cat -v, which displays non-printing characters.

opt -analyze -print-callgraph $1 >/dev/null 2>"$2".raw
cat -v "$2".raw > "$2"
