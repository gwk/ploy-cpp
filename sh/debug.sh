# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

make -s _bld/ploy-dbg
lldb -- _bld/ploy-dbg "$@"
