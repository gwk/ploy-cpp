# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e

cd $(dirname "$0")/..

lang=$1; shift;
compiler=$1; shift;
interpreter=$1; shift;

make -s _bld/prof-res-usage

for src_path in "$@"; do
  name=$(basename "$src_path")
  ext=${name##*.} # strip up to and including last dot.
  [[ "$lang" == "$ext" ]] || continue
  echo
  echo $name
  if [[ -n "$compiler" ]]; then # compile.
    bin_path="_bld/${name/./-}"
    $compiler -o "$bin_path" "$src_path"
    cmd="${bin_path//sh\/..\//}"
  else
    cmd="${interpreter//sh\/..\//} $src_path"
  fi
  for i in $(seq 0 3); do
    _bld/prof-res-usage $cmd
  done
done
