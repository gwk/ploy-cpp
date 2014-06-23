#!/usr/bin/env python3
# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

import sys
import json
import plistlib

if len(sys.argv) != 3:
  print('usage: input-path output-path', file=sys.stderr)
  sys.exit(1)

f_in = open(sys.argv[1])
f_out = open(sys.argv[2], 'wb')

obj = json.load(f_in)
plistlib.dump(obj, f_out)
