# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

if ! sh/is-product-current.sh _build/ploy src/* sh/*; then
  echo "building rel..."
  sh/build.sh
  echo "timing rel..."
fi

/usr/bin/time _build/ploy "$@"
