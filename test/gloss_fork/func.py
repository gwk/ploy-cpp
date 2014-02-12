# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

# functional utilities


def first_satisfying(predicate, iterable):
  '''
  return the first object in iterable satisfying predicate.
  raises ValueError if no object matches.
  '''
  for e in iterable:
    if predicate(e):
      return e
  raise ValueError


def last_satisfying(predicate, iterable):
  '''
  return the last object in the iterable satisfying predicate.
  raises ValueError if no object matches.
  '''
  return first_satisfying(predicate, reversed(iterable))


def count_satisfying(predicate, iterable):
  '''
  count the number of elements in the iterable satisfying predicate.
  '''
  count = 0
  for e in iterable:
    if predicate(e):
      count += 1
  return count


def group(function, iterable, count=None):
  '''
  group the objects in iterable by applying a function to each object that returns an index.
  if a count is specified, the result always has count elements,
  and IndexError is raised if function returns an out-of-bound index.
  '''
  groups = [[] for i in range(count)]

  for item in iterable:
    group_index = function(item)
    
    if group_index < 0:
      raise IndexError

    if count is None:
      while len(groups) <= group_index:
        groups.append([])
    elif group_index >= count:
      raise IndexError
    
    groups[group_index].append(item)
  
  return groups


def group_filter(function, iterable, count=0):
  '''
  group the objects in iterable by applying a function to each object that returns an index.
  if function returns index < 0, then the item is dropped from the result array.
  '''
  groups = [[] for i in range(count)]
  
  for item in iterable:
    group_index = function(item)
    
    if group_index < 0: continue
    if count:
      if group_index >= count: raise IndexError
    else:
      while len(groups) <= group_index:
        groups.append([])
    
    groups[group_index].append(item)
  return groups


def group_by_key(function, iterable):
  '''
  group the objects in iterable by applying a function to each object that returns a key.
  '''
  groups = {}
  for item in iterable:
    key = function(item)
    groups.setdefault(key, []).append(item)
  return groups


def group_filter_by_key(function, iterable):
  '''
  group the objects in iterable by applying a function to each object that returns a key.
  if function returns None, the item is dropped from the result dictionary.
  '''
  groups = {}
  for item in iterable:
    key = function(item)
    if key is None: continue
    groups.setdefault(key, []).append(item)
  return groups


def fix_is(f, a):
  while True:
    b = f(a)
    if a is b:
      return a
    a = b


def fix_eq(f, a):
  while True:
    b = f(a)
    if a == b:
      return a
    a = b


