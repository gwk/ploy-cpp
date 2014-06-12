# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
root=$(dirname $0)/../..
cd "$root"
packages=~/"Library/Application Support/Sublime Text 3/Packages"
mkdir -p "$packages/Ploy"
json-to-plist conf/atom/grammars/ploy.json "$packages/Ploy.tmlanguage"
