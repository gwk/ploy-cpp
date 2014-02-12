# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
file write convenience functions.
'''

import os as _os # for writeR indent coloring
import pprint as _pprint

import gloss_fork.desc as _desc
import gloss_fork.string as _string

import gloss_fork.io.cs as _cs
import gloss_fork.io.sgr as _sgr

from .term import window_size as _term_window_size
from gloss.string import template as _template

# basic printing

def write(file, *items, sep='', end=''):
  "write items to file; sep='', end=''"
  print(*items, sep=sep, end=end, file=file)

def writeS(file, *items):
  "write items to file; sep=' ', end=''"
  print(*items, sep=' ', end='', file=file)

def writeSS(file, *items):
  "write items to file; sep=' ', end=''"
  print(*items, sep=' ', end=' ', file=file)

def writeL(file, *items, sep=''):
  "write items to file; sep='', end='\\n'"
  print(*items, sep=sep, end='\n', file=file)

def writeSL(file, *items):
  "write items to file; sep=' ', end='\\n'"
  print(*items, sep=' ', end='\n', file=file)

def writeLL(file, *items):
  "write items to file; sep='\\n', end='\\n'"
  print(*items, sep='\n', end='\n', file=file)


# format printing

def writeF(file, format_string, *items, **keyed_items):
  "write the formatted string to file; end=''"
  print(format_string.format(*items, **keyed_items), end='', file=file)

def writeFL(file, format_string, *items, **keyed_items):
  "write the formatted string to file; end='\\n'"
  print(format_string.format(*items, **keyed_items), end='\n', file=file)


# templated format printing

def writeTF(file, template_format_string, *items, **keyed_items):
  """
  expand the format string template with template_items, then format the string; end=''.
  useful for constructing dynamic format strings.
  """
  format_string = _template(template_format_string, **keyed_items)
  writeF(file, format_string, *items, **keyed_items)

def writeTFL(file, template_format_string, *items, **keyed_items):
  """
  expand the format string template with template_items, then format the string; end='\\n'
  useful for constructing dynamic format strings.
  """
  format_string = _template(template_format_string, **keyed_items)
  writeFL(file, format_string, *items, **keyed_items)


# pretty printing
def writeP(file, *items, label=None):
  'pretty print to file.'
  if label is not None:
    file.write(label)
    file.write (': ')
  for i in items:
    file.write(_string.pretty_format(i))
    file.write('\n')


# description printing


_children_brackets = { '-' : '[]', '~' : '{}' }


def _ideas_for_writeR():
  # after indentation consumes half of page, just write inline
  if width > 0 and depth * len(indent_str) >= width // 2:
    writeR_inline(file, obj, context=context)
    file.write('\n')
    return

  for obj in stream:
    if inline: # still trying to inline
      if substream: # cannot inline; flush buffer to multiline
        file.write('\n')
        for i in inline_items:
          _write_desc_leaf(file, i, depth + 1, indent_str, False)
        inline = False
        inline_items = None
      else:
        inline_items.append(obj)
        continue

    if not inline: # write subsequent items iteratively
      if substream:
        _write_desc_stream(file, substream, width, depth + 1, indent_str, False, context)
      else:
        _write_desc_leaf(file, i, depth + 1, indent_str, False)

  if inline: # inlining succeeded; flush buffer
    writeSL(file, '', *inline_items)


def writeR(file, obj, label=None, width=None, depth_max=-1, depth_start=0,
  color_indent=None, indented=False, context=None, sort_unordered=True):
  '''
  write recursive description text.
  
  width:        target width for text layout.
  depth_max:    maximum depth of recursive write.
  color_indent: if True, use gray color for column.
  indented:     indicates that the current position has already been indented.
  context:      an arbitrary context value for class-specific writeR implementations.

  TODO: make context a dictionary mapping classes to context values;
  this would allow every obj type to expect its own context independently of others.
  '''
  
  if label is not None:
    file.write(label)
    file.write (': ')

  # by default choose width and indent_str based on file descriptor.
  if width is None:
    width = _term_window_size(file)[0]

  if color_indent or (color_indent is None and _os.isatty(file.fileno())):
    indent_str = _sgr.sgr(_sgr.G, _sgr.K2) + ' ' + _sgr.sgr(_sgr.R) + ' ' # light gray column
  else:
    indent_str = '  '

  if not indented and depth_start:
    file.write(indent_str * depth_start)

  node = _desc.DescNode(obj, context, sort_unordered)
  visited_branches = set()
  _writeR_rec(file, node, width, depth_max, depth_start, indent_str, context, sort_unordered, visited_branches)


recursion_sym = white_up_pointing_small_triangle = ' ▵\n'

def _writeR_rec(file, node, width, depth_max, depth_current, indent_str, context, sort_unordered, visited_branches):
  '''
  recursive helper for writeR.
  '''
  objId = id(node.obj)
  # text is presumed to be already indented
  file.write(node.desc)
 
  if node.isLeaf:
    file.write('\n')
    return
  if objId in visited_branches:
    file.write(recursion_sym)
    return
  if depth_max > 0 and depth_current >= depth_max:
    file.write(' …')
    try:
      file.write(len(node))
    except TypeError: # no len method
      pass
    file.write('\n')
    return

  inline = True # attempt to inline
  l = 2 * depth_current # predicted width of inlined string, starts at indent length

  visited_branches.add(objId)
  for c in node.children(context, sort_unordered):
    l += len(c.desc) + 1 # add one for the space
    # inlining criteria:
    # fits within width
    # for now, branches cannot be inlined
    if l > width or not c.isLeaf:
      inline = False
      break

  if inline:
    for c in node.children(context, sort_unordered):
      write(file, ' ', c.desc)
    file.write('\n')
  else:
    depth_next = depth_current + 1
    if len(node.desc) == 1:
      file.write(' ')
      skip_indent = True
    else:
      file.write('\n')
      skip_indent = False
    for c in node.children(context, sort_unordered):
      if skip_indent:
        skip_indent = False
      else:
        file.write(indent_str * depth_next)
      _writeR_rec(file, c, width, depth_max, depth_next, indent_str, context, sort_unordered, visited_branches)
  visited_branches.remove(objId)


def writeC(file, obj, width=None, depth_max=-1, context=None, sort_unordered=True, end=''):
  '''
  write recursive description text inline.
  
  width:        maxmimum width of output string.
  depth_max:    maximum depth of recursive write.
  context:      an arbitrary context value for class-specific writeR implementations.

  TODO: make context a dictionary mapping classes to context values;
  this would allow every obj type to expect its own context independently of others.
  '''
  
  node = _desc.DescNode(obj, context, sort_unordered)
  _writeC_rec(file, node, width, depth_max, 0, context, sort_unordered)
  file.write(end)


def writeCL(file, obj, width=None, depth_max=-1, context=None, sort_unordered=True):
  writeC(file, obj, width, depth_max, context, sort_unordered, end='\n')


def _writeC_rec(file, node, width, depth_max, depth_current, context, sort_unordered):
  '''
  recursive helper for writeC.
  '''

  if node.isLeaf:
    file.write(node.desc)
    return

  brackets = _children_brackets.get(node.desc)
  if brackets:
    file.write(brackets[0])
    omit_first_space = True
  else:
    brackets = '()'
    write(file, brackets[0], node.desc)
    omit_first_space = False
  
  if depth_max > 0 and depth_current >= depth_max:
    file.write(' …')
    try:
      file.write(len(node))
    except TypeError: # no len method
      pass
  else:
    depth_next = depth_current + 1
    first = True
    for c in node.children(context, sort_unordered):
      if omit_first_space:
        omit_first_space = False
      else:
        file.write(' ')
      _writeC_rec(file, c, width, depth_max, depth_next, context, sort_unordered)

  file.write(brackets[1])


# write to path

def write_to_path(path, *items, sep='', end=''):
  with open(path, 'w') as f:
    write(f, *items)

def write_to_pathS(path, *items):
  with open(path, 'w') as f:
    writeS(f, *items)

def write_to_pathL(path, *items):
  with open(path, 'w') as f:
    writeL(f, *items)

def write_to_pathSL(path, *items):
  with open(path, 'w') as f:
    writeSL(f, *items)

def write_to_pathLL(path, *items):
  with open(path, 'w') as f:
    writeLL(f, *items)

def write_to_pathF(path, format_string, *items, **keyed_items):
  with open(path, 'w') as f:
    writeF(f, format_string, *items, **keyed_items)

def write_to_pathFL(path, format_string, *items, **keyed_items):
  with open(path, 'w') as f:
    writeFL(f, format_string, *items, **keyed_items)

def write_to_pathR(path, obj, width=None, depth_max=-1, depth_start=0,
                   indent_str=None, indented=False, context=None, sort_unordered=True):
  with open(path, 'w') as f:
    writeR(f, obj, width, depth_max, depth_start, indent_str, indented, context, sort_unordered)

def write_to_pathC(path, obj, width=None, depth_max=-1, context=None, sort_unordered=True, end=''):
  with open(path, 'w') as f:
    writeC(f, obj, width, depth_max, context, sort_unordered, end)
