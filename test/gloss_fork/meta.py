# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

# metaprogramming functions

import sys as _sys
import keyword as _keyword

keywords = frozenset(_keyword.kwlist)


def validate_name(name):
  '''
  raise ValueError if name is not a valid field name.
  identifier ::=  (letter|"_") (letter | digit | "_")*
  '''
  es = None
  if not name:
    es = 'is empty'
  elif name in keywords:
    es = 'is a keyword'
  elif name[0].isdigit():
    es = 'begins with digit'
  elif name.startswith('__'):
    es = "begins with '__' (reserved)"
  else:
    for c in name:
      if not (c.isalnum() or c == '_'):
        es = 'contains invalid char:' + repr(c)
        break
  if es:
    raise ValueError('invalid name token: {} {}'.format(repr(name), es))
  

def frame_module_name(depth=1):
  '''
  return the __name__ string of the stack frame at depth, or '__main__' if undefined.
  depth=0 is this function's frame; depth=1 (default) is the caller's frame.
  according to comments in the collections.namedtuple implementation,
  generated classes (and probably functions) should have their __module__ variable set appropriately for pickling to work.
  note: this function uses sys._getframe, which is an implementation-specific function of CPython.
  '''
  return _sys._getframe(depth).f_globals.get('__name__', '__main__')


def frame_module(depth=1):
  '''
  return the module object of caller at the stack frame at depth. see frame_module_name().
  '''
  return _sys.modules[frame_module_name(depth + 1)]


def alias(obj, dst_name=None, dst_module=None, rename=False, depth=1):
  '''
  alias an object into dst_module (defaults to module of caller at depth).
  if no dst_name is provided, use obj.__name__.
  if rename is True, change object __name__ and __module__ to dst_name and dst_module.__name__.
  return the object as a convenience.
  '''
  if not dst_module:
    dst_module = frame_module(depth + 1)
  if not dst_name:
    dst_name = obj.__name__
  validate_name(dst_name)
  if rename:
    try:
      obj.__name__ = dst_name
      obj.__qualname__ = dst_name
    except AttributeError: # some objects have a module but no name or qualname
      pass
    obj.__module__ = dst_module
  #print('ALIAS:', dst_module, dst_name, obj)
  setattr(dst_module, dst_name, obj)
  return obj


def alias_list(alias_list, src_module=None, dst_module=None, rename=False, depth=1):
  '''
  alias objects named in alias_list from src_module to dst_module (defaults to module of caller at depth).
  each item in the list can be either a string naming the object, or a (src, dst) string pair.
  '''
  if not dst_module:
    dst_module = frame_module(depth + 1)
  for a in alias_list:
    if isinstance(a, tuple):
      # source can be either the name of an object in the source module or else an arbitrary object
      obj = getattr(src_module, a[0]) if src_module else a[0]
      dst_name = a[1]
    else: # source name or obj
      obj = getattr(src_module, a) if src_module else a
      dst_name = a if src_module else None # if None, alias() will attempt to use obj.__name__ as dst_name
    alias(obj, dst_name, dst_module, rename, depth + 1)


def generic_class(name, fields, depth=1):
  '''
  generate a generic class with attributes as specified.
  each field can be either a name string or a (name, default_value) pair.
  unlike namedtuple, fields are simple, mutable attributes.
  '''
  validate_name(name)
  names_list = []
  vals_list = []
  # validate fields
  names_set = set()
  for f in fields:
    if isinstance(f, str):
      n = f
      v = None
    else:
      n = f[0]
      v = f[1]
    if n in names_set:
      e = 'duplicate'
    else:
      validate_name(n)
    if e: raise ValueError('generic_class: {}; invalid field name: {}; {}'.format(name, n, e))
    names_list.append(n)
    vals_list.append(v)

  class c:
    field_names = tuple(names_list)
    default_values = tuple(vals_list)
    
    def __init__(self):
      for i, fn in enumerate(__class__.field_names):
        try:
          v = __class__.default_values[i]
        except IndexError:
          v = None
        setattr(self, fn, v)
    
    def items(self):
      return [(fn, getattr(self, fn)) for fn in field_names]

    def __str__(self):
      return ' '.join('{}: {};'.format(fn, v) for fn, v in self.items())
    
    def __repr__(self):
      return '{{{}}}'.format(' '.join('{}: {};'.format(fn, repr(v)) for (fn, v) in self.items()))

  c.__name__ = name
  c.__qualname__ = name
  c.__module__ = frame_module_name(depth + 1)

  return c
