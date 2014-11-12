# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

cd $(dirname "$0")/..

# this depends on the rename-enum gloss utility.
rename-enum -w 2 -off 1 -dash-separator src-boot/*.h

prev_name=''
for path in $(ls src-boot/*.h src-boot/ploy.cpp); do
  if [[ -n "$prev_name" ]]; then
    echo "fix include: $path; $prev_name"
    text-replace \
    -pattern '#include "\d+\w*-[-\w]+.h"' \
    -replacement "#include \"$prev_name\"" \
    -modify -no-backup $path
  fi
  prev_name="${path#src-boot/}"
done
