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
import subprocess
from collections import defaultdict
from pprint import pprint

def errSL(*items):
  print(*items, file=sys.stderr)


ignored_sym_re = re.compile(r'''(?x)
  null\ function
| llvm\..+
|  __assert_rtn
|  __memset_chk
''')


omitted_syms = set()

def is_omitted(sym):
  'check if a sym should be ommitted (some symbols are uninteresting).'
  if sym in omitted_syms: return True
  if ignored_sym_re.fullmatch(sym):
    errSL('omitting:', sym)
    omitted_syms.add(sym)
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

  g = defaultdict(set) # maps syms to sets of syms. TODO: use counters instead?

  with open(path) as f:
    node = None # current node.
    is_node_omitted = False
    for line in f:
      # look for a node line.
      m = node_re.match(line)
      if m: # line defines a node.
        node, addr, uses = m.groups()
        is_node_omitted = is_omitted(node)
        #errSL('NODE:', node, addr, uses, is_node_omitted)
        continue
      # look for an edge line.
      m = edge_re.match(line)
      if m: # line defines an edge.
        e = m.groups()
        callsite, callee = e
        if is_node_omitted or is_omitted(callee): continue
        g[node].add(callee)
        continue
      # ignored line.
      if not ignored_line_re.match(line):
        errSL('SKIPPED:', repr(line))
  return g


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
      syms_str = m.group(1)
      all_syms = syms_str.split(', ')
      last_sym = all_syms[-1]
      assert last_sym in ('', ' (Has self-loop).')
      syms = all_syms[:-1]
      if len(syms) == 1: continue # skip trivial SCCs.
      sccs.append(tuple(syms))
  return sccs


sym_names = {} # mangled sym -> full demangled name.
short_names = {} # short -> full|None; shorten names that are not overloaded.
sym_labels = {} # short names where possible, otherwise full names.

def map_syms(graph):
  syms = set()
  for src, sinks in graph.items():
    syms.add(src)
    syms.update(sinks)
  cmd = [path_demangle]
  cmd.extend(syms)
  demangle_out = subprocess.check_output(cmd)
  for line_bytes in demangle_out.split(b'\n'):
    line = line_bytes.decode('utf8')
    sym, p, n = line.partition(' -> ')
    short = n.partition('(')[0]
    sym_names[sym] = n
    sym_labels[sym] = short # first assume that all names can be shortened.
    if short in short_names: # overloaded; no sym gets this short name.
      short_names[short] = None
    else:
      short_names[short] = sym
  for sym, n in sym_names.items():
    if short_names[sym_labels[sym]] is None: # short is overloaded; revert to full name.
      sym_labels[sym] = sym_names[sym]

def quote(name):
  'double-quote a string; unfortunately there is no way to force repr to use double quotes.'
  assert not '"' in name
  return '"{}"'.format(name)

def label(sym):
  return quote(sym_labels[sym])


def write_scc_graphs(f, graph, sccs):
  for scc in sccs:
    f.write('\n') # extra line for distinguishing SCCs in the dot source.
    for sym in scc:
      f.write('  {};\n'.format(label(sym)))
      for e_sink in sorted(graph[sym]): # name is the edge source.
        if not e_sink in scc: continue # omit calls that are not mutually recursive.
        f.write('  {:<24} -> {};\n'.format(label(sym), label(e_sink)))


# main.

path_demangle, path_graph, path_sccs, path_out = sys.argv[1:]
graph = parse_callgraph(path_graph)
sccs = parse_sccs(path_sccs)
map_syms(graph)

# produce call graph.
with open(path_out, 'w') as f:
  f.write('''\
digraph "Mutual Recursion Call Graphs" {
  label="Mutual Recursion Call Graphs";
''')
  write_scc_graphs(f, graph, sccs)
  f.write('}\n')

