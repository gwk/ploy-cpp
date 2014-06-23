#!/usr/bin/env python3
# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

# generate a dot file from llvm opt --print-callgraph.
# this dot file is different than that produced by --dot-callgraph;
# it omits uninteresting nodes using a regex filter,
# and currently only prints graphs of strongly connected components (SCCs);
# these show the mutual recursion relationships in the program.


import re
import sys
from collections import defaultdict
from pprint import pprint


def errSL(*items):
  print(*items, file=sys.stderr)


ignored_name_re = re.compile(r'''(?x)
  null\ function
| llvm\..+
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
    errSL('omitting:', name)
    omitted_names.add(name)
    return True
  return False


def parse_callgraph(path):
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

  # read in the graph.
  nodes = [] # (name, addr, uses) tuples.
  edges = defaultdict(set) # maps node names to node name sets. TODO: use a counter instead?

  with open(path) as f:
    node_name = None # current node
    is_node_omitted = False
    for line in f:
      # look for a node line.
      m = node_re.match(line)
      #errSL(repr(line))
      if m: # line defines a node.
        n = m.groups()
        #errSL('NODE:', n)
        node_name, addr, uses = n
        is_node_omitted = is_omitted(node_name)
        if is_node_omitted: continue
        nodes.append(n)
        continue
      # look for an edge line.
      m = edge_re.match(line)
      if m: # line defines an edge.
        e = m.groups()
        #errSL('EDGE', e)
        callsite, callee = e
        if is_node_omitted or is_omitted(callee): continue
        edges[node_name].add(callee)
        continue
      # ignored line
      if not ignored_line_re.match(line):
        errSL('SKIPPED:', repr(line))
  return (sorted(nodes), edges)


def parse_sccs(path):
  # example lines:
  ex0 = 'SCCs for the program in PostOrder:\n'
  ex1 = 'SCC #1 : f, \n'
  ex2 = 'SCC #2 : f, g, \n'
  ex3 = 'SCC #3 : f,  (Has self-loop).\n'

  line_re = re.compile(r'SCC #\d+ : (.+)\n')
  assert line_re.fullmatch(ex1)
  assert line_re.fullmatch(ex2)
  assert line_re.fullmatch(ex3)

  sccs = []
  with open(path) as f:
    for line in f:
      #errSL(repr(line))
      m = line_re.match(line)
      if not m: continue
      names_str = m.group(1)
      all_names = names_str.split(', ')
      last_name = all_names[-1]
      assert last_name in ('', ' (Has self-loop).')
      names = all_names[:-1]
      if len(names) == 1: continue # skip SCCs that do not show mutual recursion relationships.
      sccs.append(names)
  return sccs


def write_scc_graphs(f, graph, sccs):
  nodes, edges = graph
  for scc in sccs:
    f.write('\n') # extra line for distinguishing SCCs in the dot source.
    for name, addr, uses in nodes:
      if not name in scc: continue
      f.write('  {};\n'.format(quote_name(name)))
      for e_sink in sorted(edges[name]): # name is the edge source.
        if not e_sink in scc: continue
        f.write('  {:<24} -> {};\n'.format(quote_name(name), quote_name(e_sink)))


path_graph, path_sccs, path_out = sys.argv[1:]
g = parse_callgraph(path_graph)
sccs = parse_sccs(path_sccs)

with open(path_out, 'w') as f:
  f.write('''\
digraph "Mutual Recursion Call Graphs" {
  label="Mutual Recursion Call Graphs";
''')
  write_scc_graphs(f, g, sccs)
  f.write('}\n')

