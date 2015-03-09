# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
[[ $(dirname "$0") == sh ]] || { echo "error: must run from root dir" 1>&2; exit 1; }

if [[ "$1" == "-dbg" ]]; then
  suffix=$1
  shift;
else
  suffix=''
fi

make -s _bld/ploy$suffix
tools/run-tests.sh _bld/ploy$suffix "$@"
