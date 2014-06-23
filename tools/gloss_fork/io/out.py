# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
std out convenience functions.
'''

import sys as _sys
import gloss_fork.io.write as _write

_std_out_enabled = True


def set_std_out_enabled(enabled):
  global _std_out_enabled
  _std_out_enabled = enabled


def out_flush():
  if _std_out_enabled:
    _sys.stdout.flush()


def out(*items, sep='', end=''):
  "write items to std out; sep='', end=''"
  if _std_out_enabled:
    print(*items, sep=sep, end=end)

def outS(*items):
  "write items to std out; sep=' ', end=''"
  if _std_out_enabled:
    print(*items, end='')

def outSS(*items):
  "write items to std out; sep=' ', end=' '"
  if _std_out_enabled:
    print(*items, end=' ')

def outL(*items, sep=''):
  "write items to std out; sep='', end='\\n'"
  if _std_out_enabled:
    print(*items, sep=sep)

def outSL(*items):
  "write items to std out; sep=' ', end='\\n'"
  if _std_out_enabled:
    print(*items)

def outLL(*items):
  "write items to std out; sep='\\n', end='\\n'"
  if _std_out_enabled:
    print(*items, sep='\n')


def outF(format_string, *items, **keyed_items):
  "write the formatted string to std out; end=''"
  if _std_out_enabled:
    _write.writeF(_sys.stdout, format_string, *items, **keyed_items)

def outFL(format_string, *items, **keyed_items):
  "write the formatted string to std out; end='\\n'"
  if _std_out_enabled:
    _write.writeFL(_sys.stdout, format_string, *items, **keyed_items)


def outTF(format_string, *items, **keyed_items):
  "write the templated formatted string to std out; end=''"
  if _std_out_enabled:
    _write.writeTF(_sys.stdout, format_string, *items, **keyed_items)

def outTFL(format_string, *items, **keyed_items):
  "write the templated formatted string to std out; end='\\n'"
  if _std_out_enabled:
    _write.writeTFL(_sys.stdout, format_string, *items, **keyed_items)


def outP(*items, label=None):
  'pretty print to std out.'
  if _std_out_enabled:
    _write.writeP(_sys.stdout, *items, label=label)


def outR(*args, **kw):
  'write hierarchical structured text to std out'
  if _std_out_enabled:
    _write.writeR(_sys.stdout, *args, **kw)


def outC(*args, **kw):
  "write compact inlined description text to std err; end=''"
  if _std_out_enabled:
    _write.writeC(_sys.stdout, *args, **kw)


def outCL(*args, **kw):
  "write compact inlined description text to std err; end='\\n'"
  if _std_out_enabled:
    _write.writeCL(_sys.stdout, *args, **kw)

