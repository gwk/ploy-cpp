# Copyright 2012 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
command line argument parsing.
'''

import sys as _sys
import functools as _ft
import re as _re

import gloss_fork.dict_util as _d
import gloss_fork.eh as _eh
import gloss_fork.io.std as _io
import gloss_fork.fs as _fs
import gloss_fork.func as _func
import gloss_fork.meta as _meta
import gloss_fork.string as _string
import gloss_fork.global_ as _global


# names that cannot be used for patterns 
reserved_names = frozenset([
  'args',
  'help',
  'all_dirs',
  'all-dirs',
  'all_paths',
  'all-paths',
  'search_re',
  'search-re',
])

# special nargs values
nargs_wildcards = frozenset(['?', '*', '+'])

# alternate name binding for type builtin so that we can shadow it in Pattern constructor.
# this is somewhat undesirable, but is done for the sake of brevity and to match existing arg parser signatures.
_type_class = type


class Pattern:
  'Pattern objects specify argument parsing patterns.'

  def __init__(self, name, nargs=1, default=None, type=str, help='undocumented'):
    # immediately rename the paramater for code clarity
    arg_type = type
    del type
    assert name not in reserved_names
    _eh.assert_type(arg_type, _type_class) # using 'type' here would collide with the named argument
    self.name = name
    self.wildcard = nargs in nargs_wildcards
    self.nargs = nargs
    if not self.wildcard:
      _eh.assert_type(nargs, int)
    # pick a reasonable default based nargs quantity
    if default is None:
      if nargs == 0:
        self.default = False
      elif nargs == 1:
        self.default = None
      else:
        self.default = []
    else:
      self.default = default
    if nargs == 0:
      _eh.assert_(arg_type == bool)
      _eh.assert_type(self.default, bool)
    elif nargs == 1:
      pass
    self.arg_type   = arg_type
    self.help       = help
    if name == '-': # TODO: change to comma?
      self.attr_name = '<DASH>'
    else:
      self.attr_name = (name[1:] if name.startswith('-') else name).replace('-', '_')
      _meta.validate_name(self.attr_name)
    self.is_key = self.name.startswith('-')

  def __repr__(self):
    return '<gloss.arg.Pattern: {}; nargs={}; default={}; arg_type={}>'.format(self.name, self.nargs, self.default, self.arg_type)

  def __lt__(self, other):
    return self.name < other.name


# helper functions to create standard patterns

def arg_dirs(nargs='+', help='directory paths', **ka):
  return Pattern('dirs', nargs=nargs, help=help, **ka)

def arg_exts(nargs='+', help='file type extensions', **ka):
  return Pattern('exts', nargs=nargs, help=help, **ka)

def arg_paths(nargs='+', default=['<stdin>'], help='file paths; defaults to "<stdin>"', **ka):
  return Pattern('paths', nargs=nargs, default=default, help=help, **ka)


def key_dirs(nargs='+', help='directory paths', **ka):
  return Pattern('-dirs', nargs=nargs, help=help, **ka)

def key_exts(nargs='+', help='file type extensions', **ka):
  return Pattern('-exts', nargs=nargs, help=help, **ka)

def key_paths(nargs='+', help='file paths', **ka):
  return Pattern('-paths', nargs=nargs, help=help, **ka)


def key_format(help='python format string', **ka):
  return Pattern('-format', help=help, **ka)

def key_pattern(help='python regex pattern', **ka):
  return Pattern('-pattern', help=help, **ka)

def key_string(help='search string', **ka):
  return Pattern('-string', help=help, **ka)

def key_replacement(help='replacement string', **ka):
  return Pattern('-replacement', help=help, **ka)


def flag(name, help='undocumented flag'):
  'generic flag helper. a flag is a key pattern with no arguments.'
  return Pattern(name, nargs=0, type=bool, help=help)

def flag_hidden(help='include hidden files'):
  return flag('-hidden-paths', help=help)

def flag_lines(help='perform operation line by line instead of across whole text'):
  return flag('-lines', help=help)

def flag_modify(help='modify files in place'):
  return flag('-modify', help=help)

def flag_no_backup(help='do not backup files before mutation'):
  return flag('-no-backup', help=help)

def flag_quiet(help='suppress standard output'):
  return flag('-quiet', help=help)


class Arguments:
  'Arguments objects represent the result of command parsing.'


  def __init__(self, args, help):
    self.args = args
    self.help = help
  
  
  def __getattr__(self, name):
    return self.args[name]


  def all_paths(self, make_abs=False, filters=[], exts=None, hidden=False):
    'generator that walks all paths and paths in directories specified on command line'
    if exts is None:
      exts = self.args.get('exts', None)
    if hidden is None:
      hidden = self.args.get('hidden', None)
    return _fs.all_paths(*self.paths, make_abs=make_abs, filters=filters, exts=exts, hidden=hidden)
  

  def all_dirs(self, make_abs=False, hidden=False):
    'generator returning all dirs specified on command line'
    return _fs.all_dirs(*self.dirs, hidden=hidden, make_abs=make_abs)
  

  def search_re(self):

    'return a regex object compiled from either -pattern or -string'
    if self.pattern:
      p = self.pattern
    elif self.string:
      p = _re.escape(self.string)
    else:
      fail('no pattern to compile')
    return _re.compile(p)


def parse(
  description,
  *patterns,
  req=[],
  req_one=[],
  min_one=[],
  max_one=[],
  req_together=[],
  dependencies=[],
  paths=False,
  push_vol=True):
  '''create an Arguments result from a description and list of Pattern objects.
  
  optional lists of names regulating the interaction between patterns:
    req:          name; name of each required pattern
    req_one:      (names, ...); exactly one pattern in each tuple is allowed
    min_one:      (names, ...); at least one pattern in each tuple is required
    max_one:      (names, ...); at most one pattern in each tuple is allowed
    req_together: (names, ...); all named patterns must appear together
    dependencies: (name|(names), name|(names)); the first set of items require the second set

  optional paths flag adds the following standard patterns:
    exts key
    hidden flag
    paths

  optional push_vol boolean indicates that vol-err flag argument should be pushed to vol_err stack automatically.
  if the need arises to set this to False, perhaps this default behavior could be improved.

  in the future, a custom source other than sys.argv could be specified.
  probably best implemented as source=None, so that by default, sys.argv[0] could be handled as a special case.

  it might also be useful to special case keys of form '-set-x', that could specify override values for boolean flags
  normally specified as '-x'. this would allow calling programs to more conveniently/verbosely specify arguments.
  the special case could also bypass the duplicate name assertion below in favor of last-flag-wins semantics.
  '''

  global results

  # perhaps constraining, but appears helpful for now
  _eh.assert_type(description,  str)
  _eh.assert_type(req,          list)
  _eh.assert_type(req_one,      list)
  _eh.assert_type(min_one,      list)
  _eh.assert_type(max_one,      list)
  _eh.assert_type(req_together, list)
  _eh.assert_type(dependencies, list)
  
  # special flag to control parsing of strange arguments
  force_val_pattern = Pattern('-arg', help='treat the following argument as a value, even if it begins with a dash.') # TODO: affected by comma change?
  force_flush_pattern = Pattern('-', help='terminate an argument list.') # TODO: change to comma?

  # not parsed normally.
  special_patterns = [
    force_flush_pattern,
    flag('-help', help='print help message and exit.'),
  ]

  std_patterns = [
    force_val_pattern,
    Pattern('-output-path', help='output path; defaults to "<stdout>".'), # set the default manually because stdout is already open
    Pattern('-vol-err', default='note', help='level of error reporting to std error (0-5: silent, error, warn, note, info, debug).'),
    flag('-quiet', help='omit writes to std out.') # code must use conventional format.out*() functions
  ]

  if paths:
    std_patterns.extend([
      key_exts(),
      flag_hidden(),
      arg_paths(),
    ])

  std_max_one = [
    ('-output-path',  '-modify'),
    ('-pattern',      '-string'),
    ('-format',       '-replacement'),
  ]

  all_patterns = std_patterns + list(patterns)
  # assert that patterns are unique
  names       = set()
  attr_names  = set()
  for p in all_patterns:
    _eh.assertF(p.name not in names and p.attr_name not in attr_names,
               'duplicate pattern name: {}; attribute name: {}', p.name, p.attr_name)
    names.add(p.name)
    attr_names.add(p.attr_name)
  # group patterns
  arg_patterns, key_patterns = _func.group(lambda p: int(p.name.startswith('-')), all_patterns, count=2)
  arg_patterns.reverse() # so that we can pop from end efficiently
  words = _sys.argv[1:]
  # initialize results to empty; fill in defaults later to facilitate duplicate detection
  res = {}
  # debug aid
  debug_word = '__DEBUG_ARG_PARSING__'
  if debug_word in words:
    err_dbg = _io.errSL
  else:
    def err_dbg(*items): pass
  # parse arguments
  current_key_pattern = None
  current_args = []
  force_val = False


  def help(*lines):
    _io.errSL('help:', _global.process_name)
    _io.errL(description, '\n')
    for p in sorted(all_patterns + special_patterns):
      _io.errFL('{:<24}  {}', p.name, p.help)
    if lines:
      _io.errLL(*lines)
    _sys.exit(1)


  # consume pattern from current_args
  def flush_pattern(p):
    err_dbg('flush_pattern:', p.name)
    nargs = p.nargs
    n = len(current_args)
    if nargs == 0:
      res[p.attr_name] = not p.default
      return
    if nargs == '?':
      nargs = max(1, n)
    elif nargs == '*':
      nargs = n
    elif nargs == '+':
      _eh.checkF(n > 0, _eh.Error, 'key requires at least one argument: {}', p.name)
      nargs = n
    else:
      _eh.checkF(n > 0, _eh.Error, 'key: {}; expects {} arguments; found: {}', p.name, nargs, n)
    typed_args = []
    for a in current_args[:nargs]:
      try:
        typed_args.append(p.arg_type(a))
      except Exception as e:
        _io.errF('could not convert argument {} to {} ({})', repr(a), p.arg_type, e)
        raise
    res[p.attr_name] = typed_args[0] if p.nargs == 1 else typed_args
    del current_args[:nargs]


  def flush_current():
    nonlocal current_key_pattern
    err_dbg('flush_current:', current_key_pattern)
    if current_key_pattern:
      flush_pattern(current_key_pattern)
      current_key_pattern = None
    while current_args:
      _eh.checkF(arg_patterns, _eh.Error, 'excess arguments: {}', _string.ellipse(repr(current_args), 64))
      flush_pattern(arg_patterns.pop())
  
  # parse words

  for word in words:
    if force_val: # interpret this word as an arg, even if it looks like a key.
      force_val = False
      err_dbg('force val:', word)
      current_args.append(word)
      continue
    if word == debug_word:
      continue
    if word == '-': # handle force_flush_pattern manually, because it is a prefix of every other key.
      flush_current()
      current_key_pattern = None
      continue
    if word == '-help':
      help() # exits
    if word.startswith('-'): # key
      err_dbg('key:', word)
      # find the pattern matching this key; word may be a unique prefix.
      key_pattern = None
      for p in key_patterns:
        if p.name.startswith(word):
          if key_pattern: # prefix is not unique
            _eh.raiseF(KeyError, 'key is not unique: {}; matches {} and {}', word, key_pattern.name, p.name)
          else:
            key_pattern = p
      if not key_pattern:
        _eh.raiseF(KeyError, 'unrecognized key: {}', word)
      
      _eh.checkF(key_pattern.name not in res,
                 _eh.DuplicateKeyError,
                'duplicate key: {}{}', key_pattern.name, '' if word == key_pattern else ' ({})'.format(word))
      
      if key_pattern == force_val_pattern:
        force_val = True
        continue # do not flush
      flush_current()
      current_key_pattern = key_pattern
    else: # not a key
      err_dbg('val:', word)
      current_args.append(word)

  flush_current()
  if push_vol and 'vol_err' in res: 
    _io.push_vol_err(res['vol_err'])
  # special handling of output_path; defer opening of file until all validation is finished
  output_path_specified = 'output_path' in res
  if not output_path_specified:
    # default args.output_path string to '<stdout>', even if -modify is set, because sys.stdout is open
    # one possible alternative is to actually close sys.stdout when modify is set, to enforce the output paradigm
    # we want to set this now, before debug reporting occurs
    res['output_path'] = '<stdout>'

  # output debug information if appropriate
  if _io.vol_err_meets('debug'):
    _io.errLD('arguments:')
    for k, v in sorted(res.items()):
      _io.errFLD('  {:24}: {}', k, v)
  
  # validate arguments
  # count the number of keys in res that are in the names tuple
  def count_names_in(names):
    _eh.assert_type(names, tuple)
    return _func.count_satisfying(lambda n: n in names, res)
  
  for name in req:
    _eh.checkS(name in res, _eh.Error, 'missing required option:', name)
  for names in req_one:
    _eh.checkS(count_names_in(names) == 1, _eh.Error, 'requires exactly one of these options:', ' '.join(names))
  for names in min_one:
    _eh.checkS(count_names_in(names) > 0, _eh.Error, 'requires at least one of these options:', ' '.join(names))
  for names in std_max_one + max_one:
    _eh.checkS(count_names_in(names) < 2, _eh.Error, 'requires at most one of these options:', ' '.join(names))
  for names in req_together:
    c = count_names_in(names)
    _eh.checkS(c == 0 or c == len(names), _eh.Error, 'requires all or none of these options:', ' '.join(names))
  
  for dependents, reqs in dependencies:
    if not isinstance(dependents, tuple):
      dependents = (dependents,)
    if not isinstance(reqs, tuple):
      reqs = (reqs,)
    for d in dependents:
      if dependent not in res:
        continue
      for r in reqs:
        _eh.checkS(r in res, _eh.Error, '{} depends on: {}', d, ', '.join(reqs))
  # set up defaults
  for p in all_patterns:
    if p.attr_name not in res:
      # TODO: figure out what the semantics are for requiring more than 0 arguments.
      #_eh.checkS(p.nargs != '+', _eh.Error, 'pattern requires at least one argument:', p.name)
      res[p.attr_name] = p.default
  err_dbg('arg res:\n', *(' {} : {}\n'.format(k, v) for k, v in sorted(res.items())))
  # configure output
  if output_path_specified:
    # TODO: support append
    _sys.stdout = _fs.open_write(res['output_path'])
  # TODO: reconsider global_args
  _global.args = Arguments(res, help)
  _io.set_std_out_enabled(not _global.args.quiet)
  return _global.args


