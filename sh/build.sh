# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

cd $(dirname "$0")/..

suffix=''
[[ "$1" == "-dbg" ]] && suffix=$1

sh/_compile.sh "$@" -o _build/ploy$suffix
