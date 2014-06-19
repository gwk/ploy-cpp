# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

name="$1"
[[ -n "$name" ]] || exit 1

if ! sh/is-product-current.sh _build/$name sh/* tools/$name.c; then
  echo "  $name"
  clang \
  -std=c11 \
  -Werror \
  -Weverything \
  -Wno-gnu \
  tools/$name.c \
  -o _build/$name
fi
