# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

if ! sh/is-product-current.sh _build/ploy src/* sh/*; then
  echo "building..."
  sh/build.sh
  echo "running..."
fi

lldb -- _build/ploy "$@"
