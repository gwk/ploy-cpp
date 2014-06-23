# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
parse dictionary literals.
'''

import ast as _ast

import gloss_fork.eh as _eh
import gloss_fork.fs as _fs


def parse_kv_string_as_dict(s):
  d = _ast.literal_eval(s)
  _eh.check_type(d, dict)
  return d


def parse_kv_string(s):
  for k, v in parse_kv_string_as_dict(s).items():
    yield (k, v)


def parse_kv_file(f):
  return parse_kv_string(f.read())


def parse_kv_path(path):
  with _fs.open_read(path) as f:
    return parse_kv_file(f)
