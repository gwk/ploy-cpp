# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
ANSI Control Sequence.

Note: additional commands are documented below.
As these are implemented, the command chars should be added to the cs_re pattern.

CSI n A CUU – Cursor Up Moves the cursor n (default 1) cells in the given direction.
  If the cursor is already at the edge of the screen, this has no effect.

CSI n B: Cursor Down
CSI n C: Cursor Forward
CSI n D: Cursor Back
CSI n E: Moves cursor to beginning of the line n (default 1) lines down.
CSI n F: Moves cursor to beginning of the line n (default 1) lines up.

CSI n G: Moves the cursor to column n.

CSI n S: Scroll whole page up by n (default 1) lines. New lines are added at the bottom. (not ANSI.SYS)
CSI n T: Scroll whole page down by n (default 1) lines. New lines are added at the top. (not ANSI.SYS)

'''

import re as _re


# ANSI control sequence indicator
CSI = '\x1B['


def cs(c, *seq):
  'control sequence string'
  return '{}{}{}'.format(CSI, ';'.join(str(i) for i in seq), c)


def cs_seq(c, seq):
  'generate a sequence of control sequence strings, one for each index.'
  return (cs(c, i) for i in seq)


# regex for detecting control sequences in strings
cs_re = _re.compile(r'\x1B\[.*?[hHJKlmsu]')


def strip_cs(s):
  'strip control sequences from a string.'
  return cs_re.sub('', s)

def len_strip_cs(s):
  'calculate the length of string if control sequences were stripped'
  l = len(s)
  for m in cs_re.finditer(s):
    l -= m.end() - m.start()
  return l


# erase: forward, backward, line
ERASE_LINE_F, ERASE_LINE_B, ERASE_LINE = cs_seq('K', range(0, 3))


CLEAR_SCREEN_F, CLEAR_SCREEN_B, CLEAR_SCREEN = cs_seq('J', range(0, 3))

CURSOR_SAVE     = cs('s')
CURSOR_RESTORE  = cs('u')

CURSOR_HIDE     = cs('?25l')
CURSOR_SHOW     = cs('?25h')

CURSOR_REPORT   = cs('6n') # not sure how to interpret the results.

ALT_ENTER = cs('?1049h')
ALT_EXIT  = cs('?1049l')


def pos(x, y):
  '''
  position the cursor.
  supposedly the 'f' suffix does the same thing.
  '''
  return cs('H', y + 1, x + 1)


def show_cursor(show_cursor):
  return CURSOR_SHOW if show_cursor else CURSOR_HIDE



