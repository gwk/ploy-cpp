# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

cd $(dirname "$0")/..

sh/_compile.sh -E -D=SKIP_LIB_INCLUDES "$@"
