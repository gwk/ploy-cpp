# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

if ! sh/is-product-current.sh _build/ploy-dbg src/* sh/*; then
  echo "building dbg..."
  sh/build.sh -dbg
  echo "running..."
fi

lldb -- _build/ploy-dbg "$@"
