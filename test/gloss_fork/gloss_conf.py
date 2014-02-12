# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

# parse .gloss configuration files

import ast as _ast

import gloss_fork.path as _path
import gloss_fork.fs as _fs
import gloss_fork.kv as _kv


def parse_conf(dir, all):
  '''
  return a (conf, root_dir) tuple:
    the aggregated conf dict from all conf files found up to the project root directory.
    the root directory is the one whose conf file contains the 'project' key.
  '''
  
  conf = {}
  root_dir = None
  
  for path in _fs.search_up_ext(dir, '.gloss', all=all):

    for k, v in _kv.parse_kv_path(path):
      conf.setdefault(k, v) # only save the first value found for a given key
      if k == 'project': # found the base project name; stop searching parent directories
        root_dir = _path.dir(path)
    if root_dir:
      break
  
  return conf, root_dir


if __name__ == '__main__':
  import sys
  import gloss_fork.io.std as _io

  conf = parse_conf(sys.argv[1], all=True)
  _io.outR(conf)
