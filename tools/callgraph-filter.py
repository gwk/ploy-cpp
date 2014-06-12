#!/usr/bin/env python3
# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

import re
import sys
from collections import defaultdict
from gloss.io.std import *


node_re = re.compile('\s*(\w+) \[.+,label="\{(.+)\}"\];')
edge_re = re.compile('\s*(\w+) -> (\w+);');

args = sys.argv[1:]
path_in = args[0]
path_out = args[1]
ignored_patterns_str = args[2]
ignored_patterns = ignored_patterns_str.split()

errSL('args:', path_in, path_out)
pprint(ignored_patterns)

# compile all the patterns for nodes to igore.
ignored_regexps = []
for p in ignored_patterns:
  try:
    r = re.compile(p)
  except Exception as e:
    errSL('bad pattern:', p)
    raise
  ignored_regexps.append(r)


def clean_name(name):
  if name.startswith('\x01'):
    errSL('WARNING: name has weird 0x01 byte (will be stripped):', repr(name))
    return name.lstrip('\x01')
  return name


def quote_name(name):
  assert not '"' in name
  return '"{}"'.format(name)


ommitted_nodes = set()

def is_ommitted(name):
  if name in ommitted_nodes: return True
  for r in ignored_regexps:
    if r.fullmatch(name):
      errSL('omitting:', name)
      ommitted_nodes.add(name)
      return True
  return False


# read in the graph.
nodes = {} # maps identifiers to labels; we will use the label as the output name/identifier.
raw_edges = defaultdict(set) # maps ident to ident sets; this effectively dedupes the pairs.

with open(path_in) as f:
  for line in f:
    m = node_re.match(line)
    #outL(repr(line))
    if m: # line defines a node.
      ident, raw_name = m.groups()
      name = clean_name(raw_name)
      if is_ommitted(name): continue
      assert not ident in nodes
      nodes[ident] = name
      continue
    m = edge_re.match(line)
    if m: # line defines an edge.
      n0, n1 = m.groups()
      if is_ommitted(n0) or is_ommitted(n1): continue
      raw_edges[n0].add(n1)
    else:
      pass

# convert identifiers in edges to names.
edges = []
for ra, rb_set in raw_edges.items():
  a = clean_name(ra)
  try:
    n0 = nodes[a]
  except KeyError:
    errSL('bad edge source:', a)
    continue
  for rb in rb_set:
    b = clean_name(rb)
    try:
      n1 = nodes[b]
    except KeyError:
      errSL('bad edge sink:', b)
      continue
    edges.append((n0, n1))

with open(path_out, 'w') as f:
  f.write('''\
digraph "Call graph" {
  label="Call graph";
''')
  for n in sorted(nodes.values()):
    f.write('  {};\n'.format(quote_name(n)))
  for a, b in sorted(edges):
    f.write('  {:<24} -> {};\n'.format(quote_name(a), quote_name(b)))
  f.write('}\n')
