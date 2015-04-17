# Copyright 2015 George King.
# Permission to use this file is granted in ploy/license.txt.

import os as _os
import os.path as _path
import shlex as _shlex
import string as _string
import subprocess as _sp
import sys



def set_defaults(d, defaults):
  for k, v in defaults.items():
    d.setdefault(k, v)
  return d


def fmt_template(template, **substitutions):
  'render a template using $ syntax.'
  t = _string.Template(template)
  return t.substitute(substitutions)


# basic printing.

def write(file, *items, sep='', end=''):
  "write items to file; sep='', end=''."
  print(*items, sep=sep, end=end, file=file)

def writeS(file, *items):
  "write items to file; sep=' ', end=''."
  print(*items, sep=' ', end='', file=file)

def writeSS(file, *items):
  "write items to file; sep=' ', end=''."
  print(*items, sep=' ', end=' ', file=file)

def writeL(file, *items, sep=''):
  "write items to file; sep='', end='\\n'."
  print(*items, sep=sep, end='\n', file=file)

def writeSL(file, *items):
  "write items to file; sep=' ', end='\\n'."
  print(*items, sep=' ', end='\n', file=file)

def writeLL(file, *items):
  "write items to file; sep='\\n', end='\\n'."
  print(*items, sep='\n', end='\n', file=file)


# format printing.

def writeF(file, fmt, *items, **keyed_items):
  "write the formatted string to file; end=''."
  print(fmt.format(*items, **keyed_items), end='', file=file)

def writeFL(file, fmt, *items, **keyed_items):
  "write the formatted string to file; end='\\n'."
  print(fmt.format(*items, **keyed_items), end='\n', file=file)


# templated format printing.

def writeTF(file, template_fmt, *items, **keyed_items):
  """
  expand the format string with keyed_items, then format the string; end=''.
  useful for constructing dynamic format strings.
  """
  fmt = fmt_template(template_fmt, **keyed_items)
  writeF(file, fmt, *items, **keyed_items)

def writeTFL(file, template_fmt, *items, **keyed_items):
  """
  expand the format string template with keyed_items, then format the string; end='\\n'
  useful for constructing dynamic format strings.
  """
  fmt = fmt_template(template_fmt, **keyed_items)
  writeFL(file, fmt, *items, **keyed_items)


def writeP(file, *items, label=None):
  'pretty print to file.'
  if label is not None:
    file.write(label)
    file.write (': ')
  for i in items:
    file.write(_string.pretty_format(i))
    file.write('\n')


# std out.

def out(*items, sep='', end=''):
  "write items to std out; sep='', end=''."
  print(*items, sep=sep, end=end)

def outS(*items):
  "write items to std out; sep=' ', end=''."
  print(*items, end='')

def outSS(*items):
  "write items to std out; sep=' ', end=' '."
  print(*items, end=' ')

def outL(*items, sep=''):
  "write items to std out; sep='', end='\\n'."
  print(*items, sep=sep)

def outSL(*items):
  "write items to std out; sep=' ', end='\\n'."
  print(*items)

def outLL(*items):
  "write items to std out; sep='\\n', end='\\n'."
  print(*items, sep='\n')

def outF(fmt, *items, **keyed_items):
  "write the formatted string to std out; end=''."
  writeF(sys.stdout, fmt, *items, **keyed_items)

def outFL(fmt, *items, **keyed_items):
  "write the formatted string to std out; end='\\n'."
  writeFL(sys.stdout, fmt, *items, **keyed_items)

def outP(*items, label=None):
  'pretty print to std out.'
  writeP(sys.stdout, *items, label=label)


# std err.

def err(*items, sep='', end=''):
  "write items to std err; sep='', end=''."
  print(*items, sep=sep, end=end, file=sys.stderr)

def errS(*items):
  "write items to std err; sep=' ', end=''."
  print(*items, sep=' ', end='', file=sys.stderr)

def errSS(*items):
  "write items to std err; sep=' ', end=''."
  print(*items, sep=' ', end=' ', file=sys.stderr)

def errL(*items, sep=''):
  "write items to std err; sep='', end='\\n'."
  print(*items, sep=sep, end='\n', file=sys.stderr)

def errSL(*items):
  "write items to std err; sep=' ', end='\\n'."
  print(*items, sep=' ', end='\n', file=sys.stderr)

def errLL(*items):
  "write items to std err; sep='\\n', end='\\n'."
  print(*items, sep='\n', end='\n', file=sys.stderr)

def errF(fmt, *items, **keyed_items):
  "write the formatted string to std err; end=''."
  writeF(sys.stderr, fmt, *items, **keyed_items)

def errFL(fmt, *items, **keyed_items):
  "write the formatted string to std err; end='\\n'."
  writeFL(sys.stderr, fmt, *items, **keyed_items)

def errP(*items, label=None):
  'pretty print to std err.'
  writeP(sys.stderr, *items, label=label)


# exceptions.

def raiseS(exception_class, *items):
  raise exception_class(join_items(items, ' '))

def raiseF(exception_class, fmt, *items, **keyed_items):
  raise exception_class(fmt.format(*items, **keyed_items))

def check_type(object, class_info):
  if not isinstance(object, class_info):
    raise TypeError('expected type: {}; actual type: {};\n  object: {}'.format(
      class_info, type(object), repr(object)))


# errors.

def fail(*items, sep=''):
  err(*items, sep=sep, end='\n')
  sys.exit(1)

def failS(*items): 
  fail(*items, sep=' ')

def failL(*items):
  fail(*items, sep='\n')

def failF(fmt, *items, **keyed_items):
  fail(fmt.format(*items, **keyed_items))


def check(condition, *items, sep=''):
  'if condition is False, raise an exception, joining the item arguments.'
  if not condition: fail(*items, sep=sep)

def checkS(condition, *items):
  if not condition: failS(*items)

def checkF(condition, fmt, *items, **keyed_items):
  if not condition: failF(fmt, *items, **keyed_items)


# file system.

def remove_dir_contents(path):
  if _path.islink(path): raiseS(OSError, 'remove_dir_contents received symlink:', path)
  l = _os.listdir(path)
  for n in l:
    p = _path.join(path, n)
    if _path.isdir(p):
      remove_dir_tree(p)
    else:
      _os.remove(p)


def remove_dir_tree(path):
  remove_dir_contents(path)
  _os.rmdir(path)


def _walk_all_paths_dirs_rec(paths, yield_files, yield_dirs, exts, hidden):
  'generate file and dir paths after filtering.'
  pairs = sorted(((1 if _path.isdir(p) else 0), p) for p in paths)
  subs = []
  for is_dir, p in pairs:
    d, n = _path.split(p)
    if not hidden and n.startswith('.'):
      continue
    if not is_dir: # file.
      if yield_files and (exts is None or _path.splitext(n)[1] in exts):
        yield p
    else: # dir.
      if yield_dirs:
        yield p
      subs.extend(_path.join(p, n) for n in _os.listdir(p))
  if subs:
    yield from _walk_all_paths_dirs_rec(subs, yield_files, yield_dirs, exts, hidden)


def walk_all_paths_dirs(*paths, make_abs=False, yield_files=True, yield_dirs=True, exts=None,
  hidden=False):
  '''
  generate file and dir paths,
  after optionally filtering by file extension and/or hidden names (leading dot).
  '''
  assert not isinstance(exts, str) # should be a sequence of strings.
  assert exts is None or all(e.startswith('.') for e in exts) # all extensions should begin with a dot.
  norm_paths = (abs_path(p) for p in paths) if make_abs else paths
  return _walk_all_paths_dirs_rec(norm_paths, yield_files, yield_dirs, exts, hidden)


def walk_all_paths(*paths, make_abs=False, exts=None, hidden=False):
  return walk_all_paths_dirs(*paths, make_abs=make_abs, yield_files=True, yield_dirs=False,
    exts=exts, hidden=hidden)


def walk_all_dirs(*paths, make_abs=False, exts=None, hidden=False):
  return walk_all_paths_dirs(*paths, make_abs=make_abs, yield_files=False, yield_dirs=True,
    exts=exts, hidden=hidden)



class Marker:
  def __init__(self, desc):
    self.desc = desc

  def __str__(self):
    return '<Marker: {}>'.format(self.desc)

  def __repr__(self):
    return '<Marker {}: {}>'.format(hex(id(self)), self.desc)


# subprocess.

PIPE = _sp.PIPE
NULL = Marker('/dev/null')


class SubprocessExpectation(Exception):
  def __init__(self, cmd, return_code, expect):
    super().__init__('Subprocess {} returned {}; expected {}'.format(cmd, return_code, expect))


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


def run_cmd(cmd, stdin, out, err, interpreter, shell, env):
  '''
  run a command and return (exit_code, std_out, std_err).
  '''
  if isinstance(cmd, str):
    cmd = _shlex.split(cmd)

  if interpreter:
    cmd = [interpreter] + cmd

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
  return run_cmd(cmd, stdin, _sp.PIPE, _sp.PIPE, interpreter, shell, env)


def runC(cmd, stdin=None, out=None, err=None, interpreter=False, shell=False, env=None):
  'run a command and return exit code.'
  assert out is not _sp.PIPE
  assert err is not _sp.PIPE
  c, o, e = run_cmd(cmd, stdin, out, err, interpreter, shell, env)
  assert o is None
  assert e is None
  return c


def runCO(cmd, stdin=None, err=None, interpreter=False, shell=False, env=None):
  'run a command and return exit code, std out.'
  assert err is not _sp.PIPE
  c, o, e = run_cmd(cmd, stdin, _sp.PIPE, err, interpreter, shell, env)
  assert e is None
  return c, o



def runCE(cmd, stdin=None, out=None, interpreter=False, shell=False, env=None):
  'run a command and return exit code, std err.'
  assert out is not _sp.PIPE
  c, o, e = run_cmd(cmd, stdin, out, _sp.PIPE, interpreter, shell, env)
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
  c, o, e = run_cmd(cmd, stdin, _sp.PIPE, _sp.PIPE, interpreter, shell, env)
  _check_exp(c, cmd, exp)
  return o, e


def runO(cmd, stdin=None, err=None, interpreter=False, shell=False, env=None, exp=None):
  'run a command and return stdout as a string.'
  assert err is not _sp.PIPE
  c, o, e = run_cmd(cmd, stdin, _sp.PIPE, err, interpreter, shell, env)
  assert e is None
  _check_exp(c, cmd, exp)
  return o


def runE(cmd, stdin=None, out=None, interpreter=False, shell=False, env=None, exp=None):
  'run a command and return stdout as a string.'
  assert out is not _sp.PIPE
  c, o, e = run_cmd(cmd, stdin, out, _sp.PIPE, interpreter, shell, env)
  assert o is None
  _check_exp(c, cmd, exp)
  return e

