# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

bld=_bld/sublime-text-Ploy
dst=~/"Library/Application Support/Sublime Text 3/Packages/Ploy"

rm -rf "$bld" "$dst"
mkdir "$bld"
tools/json-to-plist.py conf/atom/grammars/ploy.json "$bld"/Ploy.tmlanguage
cp -r "$bld" "$dst"
