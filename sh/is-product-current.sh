# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

product=$1; shift

error() {
  echo "$@"
  exit 1
}


for dependency in "$@"; do
  [[ -r "$dependency" ]] || error "is-product-current: dependency does not exist: $dependency"
  [[ "$product" -nt "$dependency" ]] || exit 1
done
