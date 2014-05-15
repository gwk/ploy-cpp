# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

if ! sh/is-product-current.sh _build/text-to-c-literal sh/* src/text-to-c-literal.c; then
  echo "  text-to-c-literal"
  clang \
  -std=c11 \
  -Werror \
  -Weverything \
  src/text-to-c-literal.c \
  -o _build/text-to-c-literal
fi

if ! sh/is-product-current.sh _build/ploy-core.h sh/* src/text-to-c-literal.c src/core.ploy; then
  echo "  ploy-core.h"
  _build/text-to-c-literal core_src < src/core.ploy > _build/ploy-core.h
  echo "  ploy"
fi

