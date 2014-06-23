# Copyright 2012 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

import sys as _sys
import collections as _collections
import gloss_fork.eh as _eh


def _sorted_if_possible(seq):
  try:
    return sorted(seq)
  except TypeError:
    return seq


def _init_helper(obj, context, sort_unordered):
  'return (desc, childStream) values according to type.'
  # do not use try/except to detect desc and descChildren implementations; it masks problems.
  if hasattr(obj, 'desc'):
    try:
      desc = obj.desc(context)
      _eh.assert_type(desc, str)
    except Exception as e:
      e.args = ('type {} does not correcty implement desc:\n  {}'.format(type(obj), e.args[0]),)
      raise e
    if not hasattr(obj, 'descChildren'):
      return (desc, None)
    try:
      children = obj.descChildren(context, sort_unordered)
    except Exception as e:
      e.args = ('type {} does not correcty implement descChildren:\n  {}'.format(type(obj), e.args[0]),)
      raise e
    return (desc, children)

  if isinstance(obj, type): # types are not iterable, but do have an __iter__ slot.
    return (obj.__qualname__, None)

  # treat these iterable types as leaves.
  # note that type objects are not iterable, but they have a slot named __iter__.
  if isinstance(obj, (str, bytes, bytearray, _collections.UserString)):
    return (repr(obj), None)

  # unordered types are labeled with a tilde
  # for now, assume that objects with an items() method are unordered maps.
  try:
    return ('~', _sorted_if_possible(obj.items()) if sort_unordered else obj.items())
  except AttributeError:
    pass
  # add other unordered iterable types here
  if isinstance(obj, (set, frozenset)): 
    return ('~', _sorted_if_possible(obj) if sort_unordered else obj)

  # ordered sequence types
  # note: we could also support the python 'sequence protocol' by checking for a __getitem__ attribute.
  if isinstance(obj, tuple):
    if len(obj) == 1:
      return ('·', obj)
    if len(obj) == 2:
      return ('∶', obj)
    if len(obj) == 3:
      return ('⁝', obj)
    else:
      return ('\u2758', obj)  # light vertical bar
  if hasattr(obj, '__iter__'):
    return ('-', obj)

  return (repr(obj), None)


class DescNode:

  '''
  lazy description generator object.

  description values are determined according to type:
  - method named desc() if defined;
  - str, bytes, bytearray are treated atomically;
  - standard containers are dealt with by type;
  - otherwise the object string.
  '''

 
  def __init__(self, obj, context, sort_unordered):
    self.obj = obj
    self.desc, self._childStream = _init_helper(obj, context, sort_unordered)

  @property
  def isLeaf(self):
    return self._childStream is None


  def __len__(self):
    return len(self._childStream)


  def children(self, context, sort_unordered):
    if not hasattr(self, '_childIter'):
      try:
        self._childIter = iter(self._childStream)
      except TypeError as e:
        e.args = ('type {} does not correcty implement descChildren:\n  {}\n  {}'.format(type(self.obj), e.args[0], self.obj),)
        raise e
      self._childNodesMemo = []
    yield from self._childNodesMemo
    while True:
      c = next(self._childIter) # next will raise StopIteration instead of this generator (equivalent).
      d = DescNode(c, context, sort_unordered)
      self._childNodesMemo.append(d)
      yield d

