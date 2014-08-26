#!/usr/bin/env python3
# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

import re
import sys
from collections import defaultdict
from pprint import pprint


def errSL(*items):
  print(*items, file=sys.stderr)


actions = [(re.compile(pat), fmt) for pat, fmt in [
  (r"Function '(.+)'", '{:24}'),
  (r'Lines executed:(.+)', 'lines:{:16}'),
  (r'Branches executed:(.+)', 'branches:{:16}'),
  (r'Taken at least once:(.+)', 'taken:{:16}'),
  (r'No calls', ''),
  (r"File 'src-boot/(.+)", '{:24}'),
  (r'.+:creating .+', ''),
]]


def process_file(f):
  group = []
  for line_raw in f:
    line = line_raw.strip()
    if not line:
      print(*group)
      group = []
      continue
    for r, fmt in actions:
      m = r.fullmatch(line)
      if m:
        line = fmt.format(*m.groups())
        break
    if line:
      group.append(line)

with open(sys.argv[1]) as f:
  process_file(f)
