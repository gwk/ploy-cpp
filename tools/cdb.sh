# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# clang compilation database generator.

cd $(dirname "$0")/..

cc=$1
out=$2

cat <<EOF > "$out"
[ { "directory": "$(pwd)",
    "command": "$("$cc" -cmd src-boot/ploy.c -o _bld/ploy)",
    "file": "src-boot/ploy.c" },

  { "directory": "$(pwd)",
    "command": "$("$cc" -cmd -dbg src-boot/ploy.c -o _bld/ploy-dbg)",
    "file": "src-boot/ploy.c" }
]
EOF
