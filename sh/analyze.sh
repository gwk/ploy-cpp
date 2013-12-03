# Copyright 2011 George King.
# Permission to use this file is granted in ploy/license.txt.

root=$(dirname "$0")/..

"$root"/sh/_compile.sh --analyze -o "$root"/build/ploy-analysis.plist
