# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# this depends on the sys-test gloss utility.

set -e
cd $(dirname "$0")/..

opt=''
if [[ "$1" == '-dbg' ]]; then
  opt=$1
  shift;
fi

# make sure that ploy can parse an empty file.
_bld/ploy$opt 'test/0-basic/empty.ploy'

tools/test.py -interpreters '.ploy' _bld/ploy$opt - "$@"
