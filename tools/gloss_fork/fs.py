# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
file system functions

some functions are simple renames of functions in os and os.path.
this is done out of naming preference, and to seperate file system functions here from string functions in gloss.path.

'''


import copy as _copy
import os as _os
import sys as _sys
import re as _re

import gloss_fork.meta as _meta
import gloss_fork.path as _path
import gloss_fork.eh as _eh
import gloss_fork.io.std as _io
import gloss_fork.func as _func
import gloss_fork.global_ as _global


_meta.alias_list([
  ('abspath',     'abs_path'),
  ('exists',      'exists'),
  ('expanduser',  'expand_user'),
  ('isdir',       'is_dir'),
  ('isfile',      'is_file'),
  ('islink',      'is_link'),
  ('ismount',     'is_mount'),
  ('lexists',     'link_exists'),
], src_module=_os.path)

_meta.alias_list([
  ('chdir',       'chdir'),
  ('listdir',     'list_dir'),
  ('mkdir',       'make_dir'),
  ('makedirs',    'make_dirs'),
  ('remove',      'remove'),
  ('rmdir',       'remove_dir'),
  ('removedirs',  'remove_dirs'),
  ('walk',        'walk'),
], src_module=_os)


gloss_dir = _os.environ.get('GLOSS_DIR', None)


std_names = set(['<stdin>', '<stdout>', '<stderr>'])


def current_dir():
  return abs_path('.')

def parent_dir():
  return abs_path('..')


def time_access(path):
  return _os.stat(path).st_atime


def time_mod(path):
  return _os.stat(path).st_mtime


def time_meta_change(path):
  return _os.stat(path).st_ctime


def is_file_not_link(path):
  return is_file(path) and not is_link(path)


def is_dir_not_link(path):
  return is_dir(path) and not is_link(path)


def open_read(path):
  if path == '<stdin>':
    if _sys.stdin.isatty():
      _io.errL('(reading from terminal stdin)')
    return _sys.stdin
  return open(path, 'r')


def open_write(path):
  if path == '<stdout>': return _sys.stdout
  if path == '<stderr>': return _sys.stderr
  return open(path, 'w')


def move(path, dest, overwrite=False):
  _eh.checkS(overwrite or not exists(dest), _eh.Error, 'destination path already exists:', dest)
  _os.rename(path, dest)


def move_modified(path, path_mod, no_backup=None):
  '''
  move a temporary, modified file into place, after optional backup move.
  TODO: change order of arguments to match move()?
  '''

  if no_backup is None:
    no_backup = _global.args.args.get('no_backup', False)

  if not no_backup:
    move(path, path_for_backup(path))
  move(path_mod, path, overwrite=no_backup)


def move_pairs(pairs, overwrite=False):
  '''
  move each file specified by a (source, destination) pair, avoiding temporary collisions.
  '''
  _eh.check(not overwrite, _eh.Error, 'overwrite option not yet supported')
  if not pairs: return
  src_l, dst_l = zip(*pairs) # unzip
  src_s = set(src_l)
  dst_s = set(dst_l)

  safe_pairs = []

  for s, d in pairs:
    if s in dst_s or d in src_s: # potentially unsafe or confusing movement; use mid name.
      m = path_for_move(s)
      move(s, m)
      p = (m, d)
    else:
      p = (s, d)
    safe_pairs.append(p)

  for s, d in safe_pairs:
    move(s, d)



def path_for_next(path):
  name = _path.name(path)
  re = _re.compile(_re.escape(name) + '-(\d+)')
  found_name = False
  index = -1

  for n in list_dir(_path.dir_or_dot(path)):
    m = re.match(n)
    if m:
      i = int(m.group(1))
      if i > index: index = i
    elif n == name:
      found_name = True
  
  _io.errFLD('path_for_next: name: {}; found: {}; index: {}', name, found_name, index)

  if index >= 0:
    next = path + '-{}'.format(index + 1)
  
  elif found_name:
    next = path + '-1'
  
  else:
    next = path
  
  _eh.checkS(not exists(next), _eh.Error, 'path already exists:', next)
  return next


def path_for_backup(path):
  return path_for_next(path + '-backup')


def path_for_mod(path):
  return path_for_next(path + '-mod')


def path_for_move(path):
  return path_for_next(path + '-move')


# path filters are functions that take (path, group) args and return a group int:
# 0 for files.
# 1 for directories.
# -1 to drop.
# a filter can return 0 to treat a directory as a file.


def drop_hidden(p, g):
  'filter out hidden paths.'
  n = _path.name(p)
  return -1 if n.startswith('.') and n not in ('.', '..') else g


def keep_exts(exts, filter_dirs=False):
  'filter out paths not ending in the given extensions.'
  s = set(exts)
  return lambda p, g: g if (g == 1 and not filter_dirs) or _path.matches_exts(p, s) else -1


def drop_exts(exts, filter_dirs=False):
  'filter out paths ending in the given extensions.'
  s = set(exts)
  return lambda p, g: g if (g == 1 and not filter_dirs) or not _path.matches_exts(p, s) else -1


def trim_exts(exts):
  'treat paths ending in the given extensions as files.'
  def _trim_exts_filter(p, g):
    return 0 if _path.matches_exts(p, set(exts)) else g
  return _trim_exts_filter


def trim_opaque_source_dir_exts():
  'treat paths with certain source directory extensions as opaque files.'
  return trim_exts(_path.opaque_source_dir_exts)


def drop_opaque_source_dir_exts():
  'drop paths with certain source directory extensions as files.'
  return drop_exts(_path.opaque_source_dir_exts, filter_dirs=True)


def list_dir_paths(dir):
  'return a list of paths for the contents in the directory.'
  return [_path.join(dir, n) for n in list_dir(dir)]


def _all_paths_dirs_rec(paths, filter_fn, yield_files, yield_dirs):
  'generate file and dir paths after filtering.'
  pg_pairs = ((p, (1 if is_dir(p) else 0)) for p in paths)
  files, dirs = _func.group_filter(filter_fn, pg_pairs, count=2)
  if yield_files:
    for fp, fg in sorted(files):
      yield fp
  for dp, dg in sorted(dirs):
    if yield_dirs:
      yield dp
    sub_paths = [_path.join(dp, n) for n in list_dir(dp)]
    for p in _all_paths_dirs_rec(sub_paths, filter_fn, yield_files, yield_dirs):
      yield p


def all_paths_dirs(*paths, make_abs=False, yield_files=True, yield_dirs=True, filters=[], exts=None, hidden=False):
  '''
  generate file and dir paths after filtering.
  arbitrary filters are supported, as well as two common filters: exts and hidden.
  filters return a group index:
   -1: drop.
    0: file.
    1: dir.
  '''
  assert not isinstance(exts, str) # should be a sequence of strings.
  assert exts is None or all(e.startswith('.') for e in exts) # all extensions should begin with a dot.
  all_filters = _copy.copy(filters) if (exts or not hidden) else filters

  if exts:
    all_filters.insert(0, keep_exts(exts))
  if not hidden:
    all_filters.insert(0, drop_hidden)

  def _filter_composition(pg_pair):
    #print(pg_pair)
    p, g_in = pg_pair
    g_comp = g_in
    for f in all_filters:
      g = f(p, g_in)
      #print(g, f)
      if g == -1:
        return -1
      elif g == 0: # any filter can downgrade to files group
        g_comp = 0
      else:
        _eh.check(g == 1, _eh.Error, 'filter return bad group index:', g, f)
    return g_comp

  norm_paths = (abs_path(p) for p in paths) if make_abs else paths
  return _all_paths_dirs_rec(norm_paths, _filter_composition, yield_files, yield_dirs)


def all_paths(*paths, make_abs=False, filters=[], exts=None, hidden=False):
  return all_paths_dirs(*paths, make_abs=make_abs, yield_files=True, yield_dirs=False, filters=filters, exts=exts, hidden=hidden)


def all_dirs(*paths, make_abs=False, filters=[], exts=None, hidden=False):
  return all_paths_dirs(*paths, make_abs=make_abs, yeild_fils=False, yield_dirs=True, filters=filters, exts=exts, hidden=hidden)


def _empty_dirs_rec(dir_path, nested):
  names = list_dir(dir_path)
  if names:
    has_files = False # detect files
    all_empty_subs = True # detect if subdirs are all empty
    for name in sorted(names):
      path = _path.join(dir_path, name)
      if not is_dir(path) or is_link(path): # count directory links as files
        has_files = True
        continue
      # path is a dir; recursively yield
      path_is_empty = False # detect if this child returned itself as empty
      for d in _empty_dirs_rec(path, nested):
        if nested and d == path: # don't bother with the compare if nested=False
          path_is_empty = True
        yield d
      if not path_is_empty:
        all_empty_subs = False
    if nested and not has_files and all_empty_subs:
      yield dir_path
  else: # no directory contents
    yield dir_path


def empty_dirs(*paths, nested=True):
 
  for path in paths:
    if not is_dir(path) or is_link(path): continue
    for d in _empty_dirs_rec(path, nested):
      yield d


def search_up(path, path_predicate, all=False):
  dir = abs_path(_path.dir_or_dot(path) if is_file(path) else path)
  results = []
  # note: may want to catch permission exceptions if all == True
  while True:
    for name in list_dir(dir):
      p = _path.join(dir, name)
      if path_predicate(p):
        if all:
          results.append(p)
        else:
          return p
    if dir == '/':
      return results if all else None
    d = _path.dir(dir)
    _eh.checkF(len(d) < len(dir), _eh.Error, "bad parent directory: {}; from child: {}", d, dir)
    dir = d


def search_up_name(path, name, all=False):
  return search_up(path, lambda p: _path.name(p) == name, all)

def search_up_file(path, name, all=False):
  return search_up(path, lambda p: _path.name(p) == name and is_file(p), all)

def search_up_dir(path, name, all=False):
  return search_Up(path, lambda p: _path.name(p) == name and is_dir(p), all)

def search_up_base_name(path, base_name, all=False):
  return search_up(path, lambda p: _path.base_name(p) == base_name, all)

def search_up_file_name(path, name, all=False):
  return search_up(path, lambda p: _path.base_name(p) == name and is_file(p), all)

def search_up_ext(path, ext, all=False):
  return search_up(path, lambda p: _path.ext(p) == ext, all)


def remove_contents(path):
  if is_link(path): raiseS(OSError, 'remove_contents encountered symlink:', path)
  l = list_dir(path)
  for n in l:
    p = _path.join(path, n)
    if is_dir(p):
      remove_rec(p)
    else:
      remove(p)


def remove_rec(path):
  remove_contents(path)
  remove_dir(path)

