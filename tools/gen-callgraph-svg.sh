# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

# splines: true|false|polyline|ortho.

dot "$1" -o "$2" -Tsvg \
-Grankdir=LR \
-Gsplines=polyline
