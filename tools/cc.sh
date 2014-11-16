# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# c compiler wrapper.

# for options see http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#Warning-Options
# TODO: clang docs link?


cd $(dirname "$0")/..

cmd_prefix=''
if [[ "$1" == "-cmd" ]]; then
  shift
  cmd_prefix=echo # just print the entire compile command instead of actually running it.
fi

if [[ "$1" == "-dbg" ]]; then
  shift
  suffix="-dbg"
else
  suffix="-rel"
fi

if [[ "$#" == 0 ]]; then
  echo "no arguments; humans use make and scripts in sh/."
  exit 1
fi

set -e

mkdir -p _bld

$cmd_prefix clang++ \
@tools/cpp.options \
@tools/cpp$suffix.options \
-ferror-limit=1 \
-I _bld \
"$@"
