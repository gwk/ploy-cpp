# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

cd $(dirname "$0")/..

set -e

sh/emit-llvm.sh

cd _build

echo
echo "analyzing..."
opt -analyze -std-link-opts -dot-callgraph ploy.llvm

echo
echo "simplifying..."
../tools/callgraph-filter.py callgraph.dot callgraph-simple.dot '
external\snode
llvm\..+
__assert_rtn
__memset_chk
'


echo
echo "rendering..."
dot callgraph-simple.dot -o callgraph-simple.svg -Tsvg \
-Grankdir=LR

open -a Chrome.app callgraph-simple.svg
