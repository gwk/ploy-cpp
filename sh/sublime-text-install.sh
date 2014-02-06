# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

src=build/sublime-text-Ploy
dst=~/"Library/Application Support/Sublime Text 3/Packages/Ploy"

rm -rf "$src" "$dst"
mkdir "$src"
json-to-plist sublime-text/ploy-syntax.json "$src"/Ploy.tmlanguage
cp -r "$src" "$dst"
