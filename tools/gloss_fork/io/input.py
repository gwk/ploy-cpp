# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

import re as _re
import sys as _sys
import os as _os

from gloss_fork.immutable import Immutable as _Immutable
from gloss_fork.trie import TrieMap as _TrieMap

class Key(_Immutable):
  
  __slots__ = ['name', 'seq']

  def __init__(self, name, seq):
    self.name = name
    self.seq = seq

  def __repr__(self):
    return self.name


# tty input symbols
# TODO: add F1-F12
ARROW_UP, ARROW_DOWN, ARROW_RIGHT, ARROW_LEFT, \
F1, F2, F3, F4, F5, F6, F7, F8, F9, F10 \
= keys_list = [Key(*p) for p in [
  ('<UP>',     b'\x1b[A'),
  ('<DOWN>',   b'\x1b[B'),
  ('<RIGHT>',  b'\x1b[C'),
  ('<LEFT>',   b'\x1b[D'),
  ('<F1>',     b'\x1bOP'),
  ('<F2>',     b'\x1bOQ'),
  ('<F3>',     b'\x1bOR'),
  ('<F4>',     b'\x1bOS'),
  ('<F5>',     b'\x1b[15~'),
  ('<F6>',     b'\x1b[17~'),
  ('<F7>',     b'\x1b[18~'),
  ('<F8>',     b'\x1b[19~'),
  ('<F9>',     b'\x1b[20~'),
  ('<F10>',    b'\x1b[21~'),
  #('<F11>',    b'\x1b[22~'), # TBD
  #('<F12>',    b'\x1b[23~'), # TBD
]]

key_seqs_to_keys = { k.seq : k for k in keys_list }

def _calc_prefixes():
  '''
  currently unused.
  an alternate method of implementing input_symbolic:
  use a byte regex to chunk the input, and another to recognize any trailing prefix.
  probably faster than the TrieMap implementation below.
  '''
  global key_seqs_re
  global key_prefixes_re
  key_seqs = tuple(key_seqs_to_keys.keys())
  prefixes = set()
  for s in key_seqs:
    for i in range(len(s) - 1):
      prefixes.add(s[:i])
  
  # use group capture to keep splitting matches in output list of re.split()
  key_seqs_re = _re.compile(b''.join([
    b'(',
    b'|'.join(_re.escape(k) for k in key_seqs),
    b')'
  ]))

  key_prefixes_re = _re.compile(b''.join([
    b'(?:',
    b'|'.join(_re.escape(p) for p in prefixes),
    b')$', # only match suffix of string
  ]))


key_trie = _TrieMap(key_seqs_to_keys)

def input_symbolic(f=_sys.stdin, decode_bytes=True):
  '''
  '''
  def convert(chunk):
    return chunk.decode() if decode_bytes else chunk

  f.flush() # flush any pending data
  fd = f.fileno() # get the file descriptor so that we can use os.read.
  # this is useful because os.read will read up to the number of bytes we specify,
  # but will only block if it cannot read any.
  data = b''
  while True:
    new_data = _os.read(fd, 8192)
    if not new_data: # done reading
      if data: # prefix was never completed
        yield convert(data)
        break # done
    data += new_data
    l = len(data)
    last_i = curr_i = 0
    while True:
      # two termination cases for inner loop: clean, and prefix. handle clean here:
      assert curr_i <= l
      if curr_i == l: # all data has been handled
        if last_i < curr_i: # yield final data chunk
          yield convert(data[last_i:curr_i])
        data = b''
        break
      # match
      try:
        match_i, value, subtrie = key_trie.match(data, curr_i)
      except KeyError: # no match at this position
        curr_i += 1
      else: # matched a seq, or matched a prefix but ran out of data
        if last_i < curr_i: # yield preceding data chunk
          yield convert(data[last_i:curr_i])
        if subtrie: # found prefix, but ran out of data
          assert match_i == l
          data = data[curr_i:] # leave the prefix data for the next iteration of outer loop
          break
        else: # found seq
          yield value
          last_i = curr_i = match_i         


if __name__ == '__main__':
  import gloss_fork.io.term as _term
  # test cbreak mode and input_symbolic
  
  with _term.set_mode(_term.CBREAK):
    for c in input_symbolic():
      print(repr(c))


