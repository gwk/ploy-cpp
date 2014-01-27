# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

root=$(dirname "$0")/..
cd "$root"

if ! sh/is-product-current.sh build/text-to-c-literal sh/* src/text-to-c-literal.c; then
  echo "  text-to-c-literal"
  clang \
  -std=c11 \
  -Werror \
  -Weverything \
  src/text-to-c-literal.c \
  -o build/text-to-c-literal
fi

if ! sh/is-product-current.sh build/ploy-core.h sh/* src/text-to-c-literal.c src/ploy-core.h; then
  echo "  ploy-core.h"
  build/text-to-c-literal core_src < src/core.ploy > build/ploy-core.h
  echo "  ploy"
fi

