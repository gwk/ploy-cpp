# Copyright 2012 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
process management.
'''

import subprocess as _sp
import shlex as _shlex
import gloss_fork.marker as _marker

PIPE = _sp.PIPE
NULL = _marker.Marker('/dev/null')


class SubprocessExpectation(Exception):
  def __init__(self, cmd, return_code, expect):
    super().__init__('Subprocess {} returned {}; expected {}'.format(cmd, return_code, expect))


def read_shebang(name_or_path):
  '''
  read the script shebang (e.g. "#!/usr/bin/env python3") from the beginning of a file.
  TODO: add caching (requires converting name_or_path to an absolute path in case cwd changes?).
  '''

  if '/' in name_or_path:
    path = name_or_path
  else:
    path = which(name_or_path)

  with open(path) as f:
    shebang = '#!'
    magic = f.read(len(shebang))
    if magic != shebang:
      return None

    interpreter = f.readline().strip()
    return interpreter


def _handle_script(cmd, interpreter):
  'prepend the interpreter for the cmd if appropriate.'
  if not interpreter:
    return cmd
  if not isinstance(interpreter, str): # interpreter is True
    interpreter = read_shebang(cmd[0]) or 'sh'
  return [interpreter] + cmd


_null_file = None
def _transform_file(f):
  global _null_file
  '''
  return the file, unless it is a special marker, in which case:
    - NULL: return opened /dev/null (reused).
  '''
  if f is NULL:
    if _null_file is None:
      _null_file = open('/dev/null', 'r+b') # read/write binary
    return _null_file
  return f


def _decode(s):
  return s if s is None else s.decode('utf-8')


def run(cmd, stdin, out, err, interpreter, shell, env):
  '''
  run a command and return (exit_code, std_out, std_err).
  '''
  if isinstance(cmd, str):
    cmd = _shlex.split(cmd)

  full_cmd = _handle_script(cmd, interpreter)

  if isinstance(stdin, str):
    f_in = _sp.PIPE
    input_bytes = stdin.encode('utf-8')
  elif isinstance(stdin, bytes):
    f_in = _sp.PIPE
    input_bytes = stdin
  else:
    f_in = _transform_file(stdin) # presume None, file, PIPE, or NULL
    input_bytes = None
  p = _sp.Popen(
    full_cmd,
    stdin=f_in,
    stdout=_transform_file(out),
    stderr=_transform_file(err),
    shell=shell,
    env=env
  )
  p_out, p_err = p.communicate(input_bytes)
  return p.returncode, _decode(p_out), _decode(p_err)


def runCOE(cmd, stdin=None, interpreter=False, shell=False, env=None):
  return run(cmd, stdin, _sp.PIPE, _sp.PIPE, interpreter, shell, env)


def runC(cmd, stdin=None, out=None, err=None, interpreter=False, shell=False, env=None):
  'run a command and return exit code.'
  assert out is not _sp.PIPE
  assert err is not _sp.PIPE
  c, o, e = run(cmd, stdin, out, err, interpreter, shell, env)
  assert o is None
  assert e is None
  return c


def runCO(cmd, stdin=None, err=None, interpreter=False, shell=False, env=None):
  'run a command and return exit code, std out.'
  assert err is not _sp.PIPE
  c, o, e = run(cmd, stdin, _sp.PIPE, err, interpreter, shell, env)
  assert e is None
  return c, o



def runCE(cmd, stdin=None, out=None, interpreter=False, shell=False, env=None):
  'run a command and return exit code, std err.'
  assert out is not _sp.PIPE
  c, o, e = run(cmd, stdin, out, _sp.PIPE, interpreter, shell, env)
  assert o is None
  return c, e


def _check_exp(c, cmd, exp):
  if exp is None:
    return
  if isinstance(exp, bool):
    if exp == bool(c):
      return
  else: # expect exact numeric code
    if exp == c:
      return
  raise SubprocessExpectation(cmd, c, exp)


def runOE(cmd, stdin=None, interpreter=False, shell=False, env=None, exp=None):
  'run a command and return stdout, stderr as strings.'
  c, o, e = run(cmd, stdin, _sp.PIPE, _sp.PIPE, interpreter, shell, env)
  _check_exp(c, cmd, exp)
  return o, e


def runO(cmd, stdin=None, err=None, interpreter=False, shell=False, env=None, exp=None):
  'run a command and return stdout as a string.'
  assert err is not _sp.PIPE
  c, o, e = run(cmd, stdin, _sp.PIPE, err, interpreter, shell, env)
  assert e is None
  _check_exp(c, cmd, exp)
  return o


def runE(cmd, stdin=None, out=None, interpreter=False, shell=False, env=None, exp=None):
  'run a command and return stdout as a string.'
  assert out is not _sp.PIPE
  c, o, e = run(cmd, stdin, out, _sp.PIPE, interpreter, shell, env)
  assert o is None
  _check_exp(c, cmd, exp)
  return e


# TODO: add generators: runGO (run-generate-out), runGE


# standard unix utils

_which_cache = {}

def which(name_or_path):
  '''
  get the result of running the 'which' utility on a string.
  this should return the path to an executable in the current PATH.
  this function caches results.
  '''
  # TODO: test caching
  p = _which_cache.get(name_or_path)
  if p is not None:
    p = runO(['which', name_or_path]).strip()
    _which_cache[name_or_path] = p
  return p

