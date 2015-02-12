# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
[[ $(dirname "$0") == sh ]] || { echo "error: must run from root dir" 1>&2; exit 1; }

if [[ $1 == '-b' ]]; then
  echo "NOTE: run.sh -b will omit core files."
  shift;
  core_files=''
else
  core_files=src-core/*
fi

make -s _bld/ploy-dbg
lldb -- _bld/ploy-dbg $core_files "$@"
