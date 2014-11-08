# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..
make -s _bld/ploy
tools/perf-test.sh ploy '' "_bld/ploy src-core/*.ploy" "$@"
