# Copyright 2011 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.


import gloss_fork.string as _string


'''
dictionary utilities
'''


def set_defaults(d, defaults):
  for k, v in defaults.items():
    d.setdefault(k, v)
  return d


def difference(a, b):
  '''
  return a dictionary containing the pairs in a but not in b
  '''
  d = {}
  for k, v in a.items():
    if k not in b:
      d[k] = v
  return d


def difference_rec(a, b):
  '''
  return a dictionary containing the pairs in a but not in b.
  any values that are dictionaries will be recursively differenced.
  '''
  d = {}
  for k, v in a.items():
    try:
      vb = b[k]
      if isinstance(v, dict) and isinstance(v, dict):
        v = difference_rec(v, vb)
        if not v: continue # empty result dict
      else: continue # v or vb is not a dict - cannot compare
    except KeyError: pass # k not in vb
    d[k] = v
  return d


def values_sorted_by_kv(d):
  return [v for k, v in sorted(d.items())]


def padded_items(d):
  if not d:
    return []

  w = max(len(k) for k in d.keys())
  return ((_string.padJL(k, w), v) for k, v in d.items())


def padded_str(d, prefix=''):
  if not d:
    return str(d)

  return ''.join('{}{} : {}\n'.format(prefix, k, v) for k, v in padded_items(d))


