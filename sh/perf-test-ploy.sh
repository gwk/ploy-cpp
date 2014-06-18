# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
root=$(dirname "$0")/..
"$root/sh/build.sh" -opt
exec "$root/sh/perf-test-bin.sh" ploy '' "$root/_build/ploy" "$@"
