# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

sh/update-tools.sh

if ! sh/is-product-current.sh _build/ploy-core.h sh/* _build/text-to-c-literal src/core.ploy; then
  echo "  ploy-core.h"
  _build/text-to-c-literal core_src < src/core.ploy > _build/ploy-core.h
  echo "  ploy"
fi

