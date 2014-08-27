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
  (r"Function '(.+)'", '{:32}'),
  (r'Lines executed:(.+%) of (.+)', 'lines: {:>7} of {:>4};    '),
  (r'Branches executed:(.+%) of (.+)', 'branches: {:>7} of {:>4};    '),
  (r'Taken at least once:(.+%) of (.+)', 'taken: {:>7} of {:>4}'),
  (r'No calls', ''),
  (r"File 'src-boot/(.+)'", '{:32}'),
  (r'.+:creating .+', ''),
]]


def print_groups(groups):
  groups.sort()
  for g in groups:
    print(*g)


def process_file(f):
  groups = []
  group = []
  in_functions = True # functions, then files.
  for line_raw in f:
    line = line_raw.strip()
    if not line:
      groups.append(group)
      group = []
      continue
    if in_functions and line.startswith('File'): # transition.
      print_groups(groups) # functions.
      groups = []
      in_functions = False
    for r, fmt in actions:
      m = r.fullmatch(line)
      if m:
        line = fmt.format(*m.groups())
        break
    if line:
      group.append(line)
  print_groups(groups) # files.

with open(sys.argv[1]) as f:
  process_file(f)
