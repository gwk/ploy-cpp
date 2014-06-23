# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
error handling.
this module defines a variety of wrapper functions that raise exceptions.
the goal is to make descriptive error handling as easy as possible for application code.
the variants are flattened out somewhat to minimize stack trace bloat.
conceptually, fail and assert are meant to be used in cases where failure is the result of a programmer error,
whereas raise and check are meant to handle errors external to the code in question.
'''

from gloss.string import join_items


# exceptions

class Error(Exception):
  'generic user error.'
  pass

class DuplicateIndexError(LookupError):
  'raised when a sequence assignment would result in an illegal overwrite of existing data.'
  pass

class DuplicateKeyError(LookupError):
  'raised when a mapping assignment would result in an illegal overwrite of existing data.'
  pass

class AmbiguousIndexError(LookupError):
  'raised when a sequence finds that an index points to an ambiguous entry (e.g. a marker indicating multiple entries).'
  pass

class AmbiguousKeyError(LookupError):
  'raised when a map finds that a key points to an ambiguous entry (e.g. a marker indicating multiple entries).'
  pass


def must_override(obj):
  'call in default method implementations that must be overridden by subclasses.'
  raise AssertionError('subclass must override: {}'.format(obj.__class__))


def must_override_classmethod(cls):
  'call in default class method implementations that must be overridden by subclasses.'
  raise AssertionError('subclass must override: {}'.format(cls))


def raise_(exception_class, *items, sep=''):
  'raises an exception, joining the item arguments.'
  raise exception_class(join_items(items, sep))

def raiseS(exception_class, *items):
  raise exception_class(join_items(items, ' '))

def raiseL(exception_class, *items):
  raise exception_class(join_items(items, '\n'))

def raiseF(exception_class, format_string, *items, **keyed_items):
  raise exception_class(format_string.format(*items, **keyed_items))


def check(condition, exception_class, *items, sep=''):
  'if condition is False, raise an exception, joining the item arguments.'
  if not condition: raise exception_class(join_items(items, sep))

def checkS(condition, exception_class, *items):
  if not condition: raise exception_class(join_items(items, ' '))

def checkL(condition, exception_class, *items):
    if not condition: raise exception_class(join_items(items, '\n'))

def checkF(condition, exception_class, format_string, *items, **keyed_items):
  if not condition: raise exception_class(format_string.format(*items, **keyed_items))

def check_type(object, class_info):
  if not isinstance(object, class_info):
    raise TypeError('expected type: {}; actual type: {};\n  object: {}'.format(class_info, type(object), repr(object)))


# assertions

def fail(*items, sep=''):
  'raise an assertion (looks nicer than assert(0).'
  raise AssertionError(join_items(items, sep, ''))

def failS(*items): 
  raise AssertionError(join_items(items, sep, ' '))

def failL(*items):
  raise AssertionError(join_items(items, sep, '\n'))

def failF(format_string, *items, **keyed_items):
  raise AssertionError(format_string.format(*items, **keyed_items))


def assert_(condition, *items, sep=''):
  'make an assertion. this friendlier than the assertion statement, which treats multiple arguments as a tuple.'
  if not condition: raise AssertionError(join_items(items, sep, ''))

def assertS(condition, *items):
  if not condition: raise AssertionError(join_items(items, sep, ' '))

def assertL(condition, *items):
  if not condition: raise AssertionError(join_items(items, sep, '\n'))

def assertF(condition, format_string, *items, **keyed_items):
  if not condition: raise AssertionError(format_string.format(*items, **keyed_items))


def assert_type(object, class_info):
  if not isinstance(object, class_info):
    raise AssertionError('expected type: {}; actual type: {};\n  object: {}'.format(class_info, type(object), repr(object)))
