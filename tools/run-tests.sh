# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

interpreter=$1; shift

# make sure that ploy can parse an empty file.
"$interpreter" 'test/0-basic/empty.ploy'

tools/test.py -interpreters '.ploy' "$interpreter" - "$@"
