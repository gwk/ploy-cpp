# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
[[ $(dirname "$0") == sh ]] || { echo "error: must run from root dir" 1>&2; exit 1; }

make -s _bld/prof-res-usage _bld/ploy
_bld/prof-res-usage _bld/ploy "$@"
