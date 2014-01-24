# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
root=$(dirname "$0")/..
cd "$root"

if ! sh/is-product-current.sh build/ploy src/* sh/*; then
  echo "building..."
  "$root"/sh/build.sh
  echo "running..."
fi

build/ploy "$@"
