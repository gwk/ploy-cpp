# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
file read convenience functions.
'''

from .err import errSL as _errSL


def read_from_path(path, default=None):
  try:
    with open(path) as f:
      return f.read()
  except FileNotFoundError:
    if default is not None:
      return default
    else:
      raise
  except Exception:
    _errSL('error reading from path:', path)
    raise


def chunks_from_file(f, chunks=4096):
  '''
  read and yield binary chunks from a file.
  f must be opened in binary mode.
  '''
  assert chunks != 0
  while True:
    chunk = f.read(chunks)
    if not chunk:
      return
    yield chunk
