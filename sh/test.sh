# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# this depends on the sys-test gloss utility.

set -e
root=$(dirname "$0")/..
cd "$root"

if ! sh/is-product-current.sh build/ploy src/* sh/*; then
  echo "building..."
  "$root"/sh/build.sh
  echo "testing..."
fi


ploy_flags=''
# TODO: ploy flags are escaped with extra dash.

test_args="$@"
[[ -z "$test_args" ]] && test_args='test'

# make sure that ploy can parse an empty file.
build/ploy 'test/0-basic/empty.ploy'

set -x
~/work/gloss/bin/sys-test.py -interpreters '.ploy' "build/ploy $ploy_flags" - \
$test_args
