# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
root=$(dirname "$0")/..
"$root/sh/test.sh" "$root"/test/4-perf
