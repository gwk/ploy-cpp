# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
root=$(dirname "$0")/..

for lang in c py ploy; do
  "$root/sh/perf-test-$lang.sh" "$@"
done
