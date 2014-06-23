#!/usr/bin/env python3
# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

# generate a dot file from llvm opt --print-callgraph.
# this dot file is simplified compared to the one you get from --dot-callgraph

import re
import sys
from collections import defaultdict
from pprint import pprint


def errSL(*items):
  print(*items, file=sys.stderr)


# example lines:
ex_n0 = 'Call graph node <<null function>><<0x7fe8f0ef8cd0>>  #uses=0\n'
ex_n1 = "Call graph node for function: 'main'<<0x7fe8f0ec37a0>>  #uses=1\n"
ex_e0 = "  CS<0x0> calls function 'main'\n"

node_re = re.compile(r"Call graph node (?:for function: )?(?:<<|')(.+)(?:'|>>)<<(\w+)>>  #uses=(\d+)\n")
edge_re = re.compile(r"  CS<(\w+)> calls function '(.+)'\n");
assert node_re.fullmatch(ex_n0)
assert node_re.fullmatch(ex_n1)
assert edge_re.fullmatch(ex_e0)

# use a verbose regex to describe the other known possibilities for lines, which we ignore.
ignored_line_re = re.compile(r'''(?x)
  \n
| CallGraph\ Root\ is:\ main\n
| \ \ CS<\w+>\ calls\ external\ node\n
''')

args = sys.argv[1:]
path_in = args[0]
path_out = args[1]

ignored_name_re = re.compile(r'''(?x)
  llvm\..+
|  __assert_rtn
|  __memset_chk
''')


def quote_name(name):
  'double-quote a name; unfortunately there is no way to force repr to use double quotes.'
  assert not '"' in name
  return '"{}"'.format(name)


omitted_names = set()

def is_omitted(name):
  'check if a name should be ommitted (some symbols are uninteresting).'
  if name in omitted_names: return True
  if ignored_name_re.fullmatch(name):
    print('omitting:', name)
    omitted_names.add(name)
    return True
  return False


# read in the graph.
nodes = {} # maps node names to (name, addr, uses) tuples.
edges = defaultdict(set) # maps node names to node name sets.

with open(path_in) as f:
  node_name = None # current node
  is_node_omitted = False
  for line in f:
    # look for a node line.
    m = node_re.match(line)
    #print(repr(line))
    if m: # line defines a node.
      n = m.groups()
      #print('NODE:', n)
      node_name, addr, uses = n
      is_node_omitted = is_omitted(node_name)
      if is_node_omitted: continue
      if (node_name in nodes):
        print('NODE DUPLICATE:', n)
        continue
      nodes[node_name] = n
      continue
    # look for an edge line.
    m = edge_re.match(line)
    if m: # line defines an edge.
      e = m.groups()
      #print('EDGE', e)
      callsite, callee = e
      if is_node_omitted or is_omitted(callee): continue
      edges[node_name].add(callee)
      continue
    # ignored line
    if not ignored_line_re.match(line):
      print('SKIPPED:', repr(line))


with open(path_out, 'w') as f:
  f.write('''\
digraph "Call graph" {
  label="Call graph";
''')
  for n in sorted(nodes.keys()):
    f.write('  {};\n'.format(quote_name(n)))
    for e_sink in sorted(edges[n]):
      f.write('  {:<24} -> {};\n'.format(quote_name(n), quote_name(e_sink)))
  f.write('}\n')
