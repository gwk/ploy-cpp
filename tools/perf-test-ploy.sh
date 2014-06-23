# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
root=$(dirname "$0")/..
make -s _bld/ploy
"$root/tools/perf-test.sh" ploy '' "$root/_bld/ploy" "$@"
