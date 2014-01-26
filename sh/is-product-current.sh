# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

product=$1; shift

for dependency in "$@"; do
  [[ "$product" -nt "$dependency" ]] || exit 1
done
