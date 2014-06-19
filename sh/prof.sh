# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

sh/update-tools.sh

if ! sh/is-product-current.sh _build/ploy src/* sh/*; then
  echo "building rel..."
  sh/build.sh
  echo "timing rel..."
fi

_build/prof-res-usage _build/ploy "$@"
