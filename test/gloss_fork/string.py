# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
string functions.
'''

import bisect as _bisect
import pprint as _pp
import string as _string

import gloss_fork.func as _func
import gloss_fork.io.sgr as _sgr

# default pretty printer
pretty_formatter = _pp.PrettyPrinter(stream=None, indent=2)

# return a formatted string representing item
def pretty_format(item):
  'return a pretty formatted string.'
  return pretty_formatter.pformat(item)


def join_items(items, sep=' ', end=''):
  'join the string representations of a list of items with a seperator and end string.'
  return sep.join(str(i) for i in items) + end


def join_dict_items(items):
  'join items as the they would appear in dict literal.'
  return ', '.join('{}: {}'.format(repr(k), repr(v)) for k, v in items)


def substring(string, span):
  'return the substring inside the range of the tuple.'
  return string[span[0]:span[1]]


def ellipse(string, length):
  'return the whole string if it fits within the argument length, or else truncate and add an ellipsis.'
  return string if len(string) <= length else string[:length - 1] + '\u2026'


def ellipse_repr(string, length):
  '''
  return the repr of the whole string if it fits within the argument length;
  otherwise truncate and add an ellipsis.
  this function properly closes quotations.
  '''
  r = repr(string)
  return r if len(r) <= length else r[:length - 2] + r[0] + '\u2026'


def strip_prefix(string, prefix, req=True):
  'remove the prefix if it exists.'
  if string.startswith(prefix):
    return string[len(prefix):]
  elif req:
    raise ValueError(string)
  return string


def strip_suffix(string, suffix, req=True):
  'remove the suffix if it exists.'
  if string.endswith(suffix):
    return string[:len(suffix)]
  elif req:
    raise ValueError(string)
  return string


def strip_first_prefix(string, prefixes, req=True):
  for p in prefixes:
    try:
      return strip_prefix(string, p, req=True)
    except ValueError:
      continue
  if req:
    raise ValueError(string)
  return string


def line_index_of_pos(string, pos):
  'get the 0-indexed line index of position in string.'
  return string.count('\n', 0, pos)

def line_number_of_pos(string, pos):
  'get the 1-indexed line index of the position in string.'
  return line_index_of_pos(string, pos) + 1


def line_start_of_pos(string, pos):
  'find the start of the line containing pos by searching backwards for a newline character.'
  for i in range(pos - 1, 0, -1):
    if string[i] == '\n':
      return i + 1
  return 0

def line_end_of_pos(string, pos):
  'find the end of the line containing pos by searching forward for a newline character.'
  end = len(string)
  # TODO: use find (faster)
  for i in range(pos, end):
    if string[i] == '\n':
      return i + 1
  return end


def line_end_of_span_end_pos(string, end_pos):
  'find the end of the line containing a span ending with end_pos (exclusive).'
  # end_pos is the first position after the spanned substring.
  # therefore we start the search from the previous character, after checking for start-of-string.
  return line_end_of_pos(string, max(end_pos - 1, 0))


def line_span_of_pos(string, pos):
  'find the span of the line containing pos.'
  return (line_start_of_pos(string, pos), line_end_of_pos(string, pos))


def line_span_of_span(string, span):
  'find the span of the line containing span.'
  return (line_start_of_pos(string, span[0]), line_end_of_span_end_pos(string, span[1]))


def line_of_pos(string, pos):
  'find the line containing pos.'
  return substring(string, line_span_of_pos(string, pos))


def lines_of_span(string, span):
  'find the lines containing span.'
  return substring(string, line_span_of_span(string, span))


def line_positions(string):
  'return an array of start-of-line positions, suitable for bisection search.'
  # begin with a line position at the first character
  a = [0]
  # find all line positions after newlines
  a.extend(pos + 1 for pos, char in enumerate(string) if char == '\n')
  # end with a terminating position after the last character
  a.append(len(string))
  return a


def line_row_col(string, line_pos_list, pos):
  '''
  return a triple containing:
    source line, stripped of newline,
    or lookup error string (does not throw an exception to ease error printing);
    row (line index);
    column (pos relative to line).
  '''
  if pos == len(string): # at the end position
    return '<EOS:{}>\n'.format(pos), -1, -1
  try:
    li = _bisect.bisect_right(line_pos_list, pos) - 1 # finds the index with largest value <= pos
    l_start = line_pos_list[li]
    l_end = line_pos_list[li + 1]
    line = string[l_start:l_end].rstrip('\n')
    return line, li, pos - l_start
  except (IndexError, ValueError):
    return '<INVALID POS:{}>'.format(pos), -1, -1


def underline(col, length, line):
  'return a string for underlining a source line.'
  assert isinstance(col, int)
  assert isinstance(length, int)
  if col < 0: # invalid
    return ''
  ll = len(line)
  lul = length
  if ll < col + length: # underline exceeds line
    lul = ll
    end = '…'
  else:
    end = ''
  return (' ' * col) + (('~' * lul) if length > 0 else '^') + end


def highlight_line_row_col(string, line_pos_list, pos, length, ul):
  l, r, c = line_row_col(string, line_pos_list, pos)
  if r < 0: # invalid
    return l, r, c
  # NOTE: have to apply SGR reset before newline because SGR rules are strange.
  e = c + length
  prefix = l[:c]
  middle = l[c:e]
  suffix = l[e:]
  strings = [
    _sgr.sgr(_sgr.G, _sgr.K6), prefix,
    _sgr.sgr(_sgr.G, _sgr.KA), middle,
    _sgr.sgr(_sgr.G, _sgr.K6), suffix,
    _sgr.sgr(_sgr.R)]
  if ul:
    strings.extend(['\n', _sgr.sgr(_sgr.TB), underline(c, length, l), _sgr.sgr(_sgr.R)])
  return ''.join(strings), r, c
  

def padJL(s, width):
  'pad a string, left-justified (append spaces)'
  return (s + ' ' * (width - len(s))) if len(s) < width else s


def padJR(s, width):
  'pad a string, right-justified (prepend spaces)'
  return (' ' * (width - len(s))) + s if len(s) < width else s


# string templating

def template(template, **substitutions):
  'render a template using $ syntax.'
  t = _string.Template(template)
  return t.substitute(substitutions)


# this probably belongs somewhere else
def char64(i):
  'character for RFC 4648 base64url encoding.'
  if i < 0:
    raise ValueError
  
  if i < 10:
    o = 48
  elif i < 36:
    o = 55
  elif i < 62:
    o = 61
  elif i == 62:
    return '-'
  elif i == 63:
    return '_'
  else:
    raise ValueError

  return chr(o + i)


def char64_lax(i):
  'base64url char or out of range symbols.'
  try:
    return char64(i)
  except ValueError:
    return '<' if i < 0 else '>'

