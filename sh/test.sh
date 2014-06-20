# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# this depends on the sys-test gloss utility.

set -e
cd $(dirname "$0")/..

opt=''
test_paths=''

if [[ "$1" == '-dbg' ]]; then
  opt=$1
  shift;
fi

if [[ -z "$@" ]]; then
  echo "defaulting to debug tests..."
  opt='-dbg'
  test_paths=test/[0-3]-* # omit the perf tests, which take too long in debug mode.
else
  test_paths="$@"
fi

sh/update-tools.sh

if ! sh/is-product-current.sh _build/ploy$opt src/* sh/*; then
  echo "building $opt..."
  sh/build.sh $opt
  echo "testing $opt..."
fi

# make sure that ploy can parse an empty file.
_build/ploy$opt 'test/0-basic/empty.ploy'

test/test.py -interpreters '.ploy' _build/ploy$opt - $test_paths
