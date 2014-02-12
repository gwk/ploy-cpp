# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

# path functions

import sys

import os.path as _os_path
import gloss_fork.meta as _meta


# extensions indicating source code directories that should not be recursed
opaque_source_dir_exts = [
  '.xcodeproj',
  '.xib',
]


# some functions are simple renames of functions in os.path
# this is done out of naming preference, and to seperate string and file system functions (see gloss.fs)
_meta.alias_list([
  ('basename',      'name'), # the file name, split from the dir
  ('commonprefix',  'common_prefix'),
  ('dirname',       'dir'), # the dir name, split from the file
  ('isabs',         'is_abs'),
  ('join',          'join'),
  ('normpath',      'norm'),
  ('relpath',       'rel'),
  ('split',         'split_dir'),
  ('splitext',      'split_ext'),
], src_module=_os_path)


def dir_or_dot(path):
  d = dir(path)
  return d if d else '.'

def base(path):
  'the path without extension'
  return split_ext(path)[0]

def ext(path):
  'the file extension of the path'
  return split_ext(path)[1]

def base_name(path):
  'the file name without extension'
  return base(name(path))


def split_dir_ext(path):
  d, ne = split_dir(path)
  n, e = split_ext(path)
  return d, n, e


def matches_exts(path, exts):
  return ext(path) in exts


def rel_to_process_dir(*components):
  return rel(join(sys.argv[0], '..', *components))

