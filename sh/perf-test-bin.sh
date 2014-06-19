# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e

root=$(dirname "$0")/..

lang=$1; shift;
compiler=$1; shift;
interpreter=$1; shift;

sh/update-tools.sh

for src_path in "$@"; do
  name=$(basename "$src_path")
  ext=${name##*.} # strip up to and including last dot.
  [[ "$lang" == "$ext" ]] || continue
  echo
  echo $name
  if [[ -n "$compiler" ]]; then # compile.
    bin_path="$root/_build/${name/./-}"
    $compiler -o "$bin_path" "$src_path"
    cmd="${bin_path//sh\/..\//}"
  else
    cmd="${interpreter//sh\/..\//} $src_path"
  fi
  res="$root/_build/perf-res-${name/./-}.txt"
  [[ -f "$res" ]] && rm "$res"
  for i in $(seq 0 3); do
    _build/prof-res-usage $cmd 2>> "$res"
  done
  cat "$res"
done
