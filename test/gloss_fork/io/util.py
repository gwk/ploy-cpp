# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

# io utilities

import gloss_fork.eh as _eh


def read_until(f, marker, read_size=1024):
  _eh.assertF(len(marker) * 2 < read_size, "marker is too long ({}) for read size: {}", len(marker), read_size)
  
  start_pos = f.tell()

  string = ''
  current = ''

  while True:
    next = f.read(read_size)
    
    if not next: # EOF; return remainder
      return string + current
    
    current += next

    i = current.find(marker)
    
    if i > -1: # found marker
      string += current[:i + len(marker)] # include marker in the result
      f.seek(start_pos + len(string)) # rewind file to match end of marker
      return string
    
    else: # didn't find marker; advance
      # since marker might have been split, cannot advance all the way
      # marker could be missing a single char from end of current
      string += current[:1 - len(marker)]
      current = current[1 - len(marker):]

