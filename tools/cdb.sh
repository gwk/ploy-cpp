# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# clang compilation database generator.

cd $(dirname "$0")/..

cat <<EOF > _bld/compile_commands.json
[ { "directory": "$(pwd)",
    "command": "$(tools/cc.sh -cmd src/ploy.c -o _bld/ploy)",
    "file": "src/ploy.c" },

  { "directory": "$(pwd)",
    "command": "$(tools/cc.sh -cmd -dbg src/ploy.c -o _bld/ploy-dbg)",
    "file": "src/ploy.c" }
]
EOF
