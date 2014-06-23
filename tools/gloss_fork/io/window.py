# Copyright 2011 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
terminal window functions.
separate from io.term to prevent dependency cycle with io.write.writeR.
'''

import itertools as _iter
import sys as _sys

import gloss_fork.io.cs as _cs
from .out import out as _out
from .out import outL as _outL
from .out import out_flush as _out_flush


def clear_screen():
  'clear screen.'
  _out(_cs.CLEAR_SCREEN)


def show_cursor(show_cursor):
  'toggle cursor visibility.'
  _out(_cs.show_cursor(show_cursor))


def pos(x, y):
  'change cursor position.'
  _out(_cs.pos(x, y))


class fullscreen:
  '''
  context manager for entering and exiting full screen.
  '''
  
  def __init__(self, show_cursor=True, disabled=False):
    self.show_cursor = show_cursor
    self.disabled = disabled

  def __enter__(self):
    if self.disabled: return
    _out(_cs.ALT_ENTER)
    show_cursor(self.show_cursor)

  def __exit__(self, exc_type, exc_val, exc_tb):
    if self.disabled: return
    show_cursor(True)
    _out(_cs.ALT_EXIT)


def paint(win_size, fn):
  '''
  paint the screen (assumes full screen mode).
  fn takes x, y coord arguments, and should return a fixed-width string (typically 1 or 2 characters).
  flushes std out when all coordinates have been painted.
  '''
  for y in range(win_size[1]):
    pos(0, y)
    _out(*(fn(x, y) for x in range(win_size[0])))
  _out_flush()


def test_paint_hex(win_size, rounds=256):
  for i in range(rounds):
    paint(win_size, lambda c: '{:X}'.format(i % 16))
