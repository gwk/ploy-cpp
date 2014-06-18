# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
root=$(dirname "$0")/..
exec "$root/sh/perf-test-bin.sh" c 'clang -Ofast' '' "$@"
