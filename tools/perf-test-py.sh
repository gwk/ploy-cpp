# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..
tools/perf-test.sh py '' python3 "$@"
