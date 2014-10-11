  # Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
std err convenience functions.

verbosity levels:
0: silent
1: errE: error
1: errW: warn
2: errN: note
3: errI: info
4: errD: debug

unix syslog defines the following:
0: Emergency emerg
1: Alert alert
2: Critical crit
3: Error err
4: Warning warning
5: Notice  notice
6: Informational info
7: Debug debug
A common mnemonic used to remember the syslog levels down to top is: "Do I Notice When Evenings Come Around Early".
'''

import sys as _sys
import traceback as _traceback

import gloss_fork.meta as _meta
import gloss_fork.io.write as _write

_stderr = _sys.stderr
# std err volume
_vol_errs = [3] # stack starts with 'warn'

vol_err_names = ('silent', 'error', 'warn', 'note', 'info', 'debug')
vol_err_silent, vol_err_error, vol_err_warn, vol_err_note, vol_err_info, vol_err_debug = range(len(vol_err_names)) # constants
_vol_err_suffixes = [n[0].upper() for n in vol_err_names]

# generate a short list of aliases for the volume names
_vol_err_aliases = {}
for _level, _name in enumerate(vol_err_names):
  for _alias in (_level, str(_level), _name[0], _name[0].upper(), _name):
    _vol_err_aliases[_alias] = _level


def vol_err():
  'the current error volume'
  return _vol_errs[-1]

def vol_err_meets(v):
  return _vol_errs[-1] >= _vol_err_aliases[v]

def push_vol_err(vol):
  'push an error volume onto the stack'
  _vol_errs.append(_vol_err_aliases[vol])


def push_limit_vol_err(vol):
  '''
  limit the volume of a section of code.
  pushes an error volume onto the stack if it is less than the current volume and the target volume is less than debug;
  otherwise push current volume.
  '''
  v = _vol_err_aliases[vol]
  c = vol_err() # current
  push_vol_err(v if v < c and v < 5 else c)


def pop_vol_err():
  'pop an error volume off the stack (not returned)'
  _vol_errs.pop()
  assert _vol_errs, 'excessive/unmatched pop'


class set_vol_err():
  '''
  context manager for setting vol_err.
  usage:
    with set_vol_err(vol):
      ...
  '''

  def __init__(self, vol):
    self.vol = vol

  def __enter__(self):
    push_vol_err(self.vol)

  def __exit__(self, xc_type, exc_val, exc_tb):
    pop_vol_err()


class limit_vol_err():
  '''
  context manager for limiting vol_err.
  usage:
    with limit_vol_err(vol):
      ...
  '''
  
  def __init__(self, vol):
    self.vol = vol

  def __enter__(self):
    push_limit_vol_err(self.vol)

  def __exit__(self, xc_type, exc_val, exc_tb):
    pop_vol_err()


def err_flush():
  _stderr.flush()


def err(*items, sep='', end=''):
  "write items to std err; sep='', end=''."
  print(*items, file=_stderr, sep=sep, end=end)

def errS(*items):
  "write items to std err; sep=' ', end=''."
  print(*items, file=_stderr, end='')

def errSS(*items):
  "write items to std err; sep=' ', end=''."
  print(*items, file=_stderr, end=' ')

def errL(*items, sep=''):
  "write items to std err; sep='', end='\\n'."
  print(*items, file=_stderr, sep=sep)

def errSL(*items):
  "write items to std err; sep=' ', end='\\n'."
  print(*items, file=_stderr)

def errLL(*items):
  "write items to std err; sep='\\n', end='\\n'."
  print(*items, file=_stderr, sep='\n')

def errF(format_string, *items, **keyed_items):
  "write the formatted string to std err; end=''."
  _write.writeF(_stderr, format_string, *items)

def errFL(format_string, *items, **keyed_items):
  "write the formatted string to std err; end='\\n'."
  _write.writeFL(_stderr, format_string, *items, **keyed_items)


def errTF(format_string, *items, **keyed_items):
  "write the templated formatted string to std err; end=''."
  _write.writeTF(_stderr, format_string, *items, **keyed_items)

def errTFL(format_string, *items, **keyed_items):
  "write the templated formatted string to std err; end='\\n'."
  _write.writeTFL(_stderr, format_string, *items, **keyed_items)


def errP(*items, label=None):
  'pretty print to std out.'
  _write.writeP(_stderr, *items, label=label)


def errR(*args, **kw):
  'write structured description text to std err.'
  _write.writeR(_stderr, *args, **kw)


def errC(*args, **kw):
  "write compact inlined description text to std err; end=''."
  _write.writeC(_stderr, *args, **kw)


def errCL(*args, **kw):
  "write compact inlined description text to std err; end='\\n'."
  _write.writeC(_stderr, *args, **kw)


# stack trace printing


def err_stack_trace(*items, sep=''):
  'print a stack trace to std err.'
  if items:
    print(*items, sep=sep, end='\n', file=_stderr)
  _traceback.print_stack(file=_stderr)


def err_exception_trace(e, *items, sep=''):
  "print an exception's stack trace to std err."
  if items:
    print(*items, sep=sep, end='\n', file=_stderr)
  _traceback.print_exception(type(e), e, e.__traceback__, limit=None, file=_stderr, chain=True)


def def_err_variants(err_fn, depth=1):
  'dynamically define standard gloss formatting functions'
  # note that factory functions are used to define and bind all functions produced by iterators.
  # otherwise the produced functions would be identical, due to the iteration variable changing value (?).
  # this problem can also be worked around by using lambda args.
  # note: functools.partial was not used so as to generate real functions with names and modules.
  def make_name(suffix):
    name_parts = list(err_fn.__name__.partition('_'))
    name_parts.insert(1, suffix)
    return ''.join(name_parts)

  def make_errV():
    'make an err function volume variant V that takes a threshold int as first argument.'
    def _errV(threshold, *args, **kw):
      if vol_err() >= threshold:
        err_fn(*args, **kw)
    _errV.__doc__ = err_fn.__doc__ + ' no-op if current err volume is less than the threshold argument.'
    return _errV, make_name('V')

  def make_errX(suffix, threshold):
    'make an err function volume variant, where the function name suffix indicates the volume threshold.'
    def _errX(*args, **kw):
      if vol_err() >= threshold:
        err_fn(*args, **kw)
    _errX.__doc__ = err_fn.__doc__ + ' volume threshold = {}.'.format(threshold)
    return _errX, make_name(suffix)

  # generate err volume variants
  fns = [make_errV()]
  for threshold, suffix in enumerate(_vol_err_suffixes):
    if threshold == 0: continue # skip 'silent'
    fns.append(make_errX(suffix, threshold))
  _meta.alias_list(fns, src_module=None, dst_module=None, rename=True, depth=depth + 1) # alias into the calling module


def def_err_variants_list(err_fns, depth=1):
  for f in err_fns:
    def_err_variants(f, depth=depth + 1)

# generate functions on module import
def_err_variants_list([
  err, errS, errSS, errL, errSL, errLL,
  errF, errFL, errTF, errTFL,
  errP, errR, errC, errCL])


if __name__ == '__main__':
  # dump the module contents
  module = _sys.modules[__name__] # get this module  
  for k in sorted(module.__dict__.keys()):
    if k.startswith('_'): continue
    v = getattr(module, k)
    try:
      print('{:16} : {:>48} {}'.format(k, v, v.__module__))
    except:
      print(k)
