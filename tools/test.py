#!/usr/bin/env python3
# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.


import argparse
import ast
import os
import os.path as _path
import re
import shlex
import signal
import string as _string
import subprocess
import sys


from base import *


bar_width = 64
results_dir = '_bld/test-results'

arg_parser = argparse.ArgumentParser(description='test harness for command line tools.')
arg_parser.add_argument('-compiler', help='compiler command string')
arg_parser.add_argument('-interpreter', help='compiler command string')
arg_parser.add_argument('-timeout', type=int, help='subprocess timeout')
arg_parser.add_argument('-parse', action='store_true', help='parse test cases and exit'),
arg_parser.add_argument('-fast',  action='store_true', help='exit on first error'),
arg_parser.add_argument('-dbg', action='store_true', help='debug mode: print extra info; propogate exceptions; implies -fast)'),
arg_parser.add_argument('paths', nargs='*', help='test paths and/or directories to search')

args = arg_parser.parse_args()

dbg = args.dbg

if dbg:
  fail_fast = True
  print("DEBUG")
else:
  fail_fast = args.fast


def lex(string):
  return shlex.split(string)

def jsq(words):
  'join lists with spaces, quoting elements.'
  return words if isinstance(words, str) else ' '.join(repr(w) for w in words)

compiler = lex(args.compiler) if args.compiler else None
interpreter = lex(args.interpreter) if args.interpreter else None

# file checks

def compare_equal(exp, val):
  return exp == val

def compare_contains(exp, val):
  return val.find(exp) != -1

def compare_match(exp, val):
  return re.match(exp, val)

def compare_search(exp, val):
  return re.search(exp, val)

def compare_ignore(exp, val):
  return True


file_checks = {
  'equal'   : compare_equal,
  'contain' : compare_contains,
  'match'   : compare_match,
  'search'  : compare_search,
  'ignore'  : compare_ignore,
}

case_defaults = {
  'args'        : [],
  'cmd'         : None,
  'code'        : 0,
  'compiler'    : None,
  'env'         : None,
  'err'         : '',
  'files'       : {}, # files is a dict mapping file path to expectation or (file-checks-key, expectation)
  'ignore'      : False,
  'in'          : None,
  'interpreter' : None,
  'out'         : '',
  'src'         : None,
  'timeout'     : 4,
}


def run_cmd(cmd, timeout, exp_code, in_path, out_path, err_path, env):
  'run a subprocess; return True if process completed and exit code matched expectation'

  # print verbose command info formatted as shell commands for manual repro
  if dbg:
    if env:
      errSL('env:', *['{}={};'.format(*p) for p in env.items()])
    errSL('cmd:', *(cmd + ['<{} # 1>{} 2>{}'.format(in_path, out_path, err_path)]))
  
  # hacky, precautionary check to avoid weird overwrites; TODO: improve/remove?
  for word in cmd:
    if word in [out_path, err_path]:
      errL('WARNING: word in cmd is a standard output capture file path:', word)
  
  # open outputs, create subprocess
  with open(in_path, 'r') as i, open(out_path, 'w') as o, open(err_path, 'w') as e:
    proc = subprocess.Popen(cmd, stdin=i, stdout=o, stderr=e, env=env)
    # timeout alarm handler
    # since signal handlers carry reentrancy concerns, do not do any IO within the handler
    timed_out = False
    def alarm_handler(signum, current_stack_frame):
      nonlocal timed_out
      timed_out = True
      proc.kill()

    signal.signal(signal.SIGALRM, alarm_handler) # set handler
    signal.alarm(timeout) # set alarm
    code = proc.wait() # wait for process to complete; change to communicate() for stdin support
    signal.alarm(0) # disable alarm
    
    if timed_out:
      outFL('test process timed out ({} sec) and was killed', timeout)
      return False
    if code != exp_code:
      outFL('test process returned bad code: {}; expected {}', code, exp_code)
      return False
    return True


def check_file_exp(path, mode, exp):
  'return True if file at path meets expectation.'
  if dbg: errFL('check_file_exp: path: {}; mode: {}; exp: {}', path, mode, repr(exp))
  try:
    with open(path) as f:
      val = f.read()
  except Exception as e:
    outSL('error reading test output file:', path)
    outSL('exception:', e) 
    outSL('-' * bar_width)
    return False
  if file_checks[mode](exp, val):
    return True
  outFL('output file {} does not {} expectation:', repr(path), mode)
  for line in exp.split('\n'):
    #outL(sgr(GB), line, sgr(RG))
    outL(line)
  if mode == 'equal': # show a diff.
    exp_path = path + '-expected'
    with open(exp_path, 'w') as f: # write expectation to file.
      f.write(exp)
    args = [exp_path, path]
    diff_cmd = ['dl'] + args
    outSL(*diff_cmd)
    _ps.runC(diff_cmd, interpreter='sh')
  else:
    outSL('cat', path)
    with open(path) as f:
      for line in f:
        l = line.rstrip('\n')
        outL(sgr(GR), l, sgr(RG))
        if not line.endswith('\n'):
          outL('(missing final newline)')
  outSL('-' * bar_width)
  return False


def check_cmd(cmd, timeout, exp_code, exp_triples, in_path, std_file_prefix, env):
  'run a command and check against file expectations; return True if all expectations matched'
  out_path = std_file_prefix + 'out'
  err_path = std_file_prefix + 'err'
  code_ok = run_cmd(cmd, timeout, exp_code, in_path, out_path, err_path, env)
  # use a list comprehension to force evaluation of all triples; avoid break on first failure.
  files_ok = all([check_file_exp(*t) for t in exp_triples])
  return code_ok and files_ok


def make_exp_triples(test_dir, out_val, err_val, files, expand_fn):
  'each expectation is structured as (key, mode, expectation).'
  exps  = { 'out' : out_val, 'err' : err_val }
  for k in exps:
    checkS(k not in files, 'file expectation shadowed by', k)
  set_defaults(exps, files)
  exp_triples = []
  for k, v in sorted(exps.items()):
    path = os.path.join(test_dir, k)
    mode, exp = ('equal', v) if isinstance(v, str) else v
    if mode == 'equal':
      exp = expand_fn(exp)
    exp_triples.append((path, mode, exp))
  if dbg: errLL('file exp triples:', *exp_triples)
  return exp_triples


def run_case(case_path, case):
  'execute the test at path in the current directory'
  outSL('executing:', case_path)
  # because we recreate the dir structure in the test results dir, parent dirs are forbidden.
  checkS(case_path.find('..') == -1, "case path cannot contain '..':", case_path)
  src_dir, file_name = _path.split(case_path)
  case_name = _path.splitext(file_name)[0]
  test_dir = _path.join(results_dir, src_dir, case_name) # output directory.
  # setup directories.
  if dbg: errSL("setting up test directory:", test_dir)
  if _path.exists(test_dir):
    remove_dir_contents(test_dir)
  else:
    _os.makedirs(test_dir)
  # setup test values.
  if dbg: errLL(*('  {}: {}'.format(k, repr(v)) for k, v in sorted(case.items())))

  test_env_vars = {    
    'SRC_DIR' : src_dir,
    'COMPILER' : jsq(compiler or 'NO-COMPILER'),
    'INTERPRETER' : jsq(interpreter or 'NO-INTERPRETER'),
  }
  if dbg:
    errL('test env vars:')
    errLL(*('  {}: {}'.format(k, repr(v)) for k, v, in sorted(test_env_vars.items())))

  def expand(string):
    'test environment variable substitution; uses string template $ syntax.'
    t = _string.Template(string)
    return t.substitute(**test_env_vars)

  def expand_poly(val):
    'expand either a string or a list; polymorphic for ease of calling below.'
    if not val:
      return val
    if isinstance(val, str):
      return lex(expand(val))
    return [expand(str(v)) for v in val]
  
  if case['in']:
    in_string = case['in']
    in_path = _path.join(test_dir, 'in')
    write_to_path(in_path, in_string)
  else:
    in_path = '/dev/null'

  if dbg: errSL('input path:', in_path)

  exp_code = case['code']
  checkF(isinstance(exp_code, (int, bool)), 'code {!r} has bad type: {!r}', exp_code, exp_code)
  timeout = case['timeout']
  checkF(isinstance(timeout, int) and timeout > 0, 'bad timeout: {!r}; type: {}',
    timeout, type(timeout))
  args, cmd, env, src_list = (expand_poly(case[k]) for k in ('args', 'cmd', 'env', 'src'))
  exp_triples = make_exp_triples(test_dir, case['out'], case['err'], case['files'], expand)
  
  if not cmd:
    if not src_list: # find default sources.
      all_files = os.listdir(src_dir)
      def is_src(n):
        base, ext = _path.splitext(n)
        return base == case_name and ext not in ('.test', '.h')
      src_list = [_path.join(src_dir, n) for n in all_files if is_src(n)]
      if dbg: errSL('default src:', *src_list)
      checkF(len(src_list) == 1, 'test case name matches multiple source files: {}', src_list)
   
    _compiler = case.get('compiler')
    if not _compiler:
      _compiler = compiler

    if _compiler:
      fail("compilers not yet supported")
      rel_src_list = [os.path.join(src_dir, s) for s in src_list]
      compile_cmd = _compiler + ['-o', name] + rel_src_list
      ok = check_cmd(
        compile_cmd,
        timeout,
        compile_exp,
        self.compile_exps,
        std_file_prefix=(test_dir + '/compile_'),
        env=env
      )
      if not ok or exp.startswith('compile'):
        return ok
      self.cmd = ['./' + self.name]

    else:
      _interpreter = case.get('interpreter')
      if not _interpreter:
        _interpreter = interpreter
      checkS(len(src_list) == 1, 'multiple interpreted sources:', src_list)
      cmd = [src_list[0]]
      checkS(_interpreter, 'no interpreter')
      cmd = _interpreter + cmd

  check(cmd, 'no cmd for execution test')
  
  # run test.
  ok = check_cmd(
    cmd + args,
    timeout,
    exp_code,
    exp_triples,
    in_path=in_path,
    std_file_prefix=(test_dir + '/'),
    env=env,
  )

  return ok

  
def read_case(test_path):
  'read the test file.'
  with open(test_path) as f:
    s = f.read()
  case = ast.literal_eval(s)
  check_type(case, dict)
  for k in case:
    if k not in case_defaults:
      errSL('WARNING: bad test case key:', k)
  set_defaults(case, case_defaults)
  return case


# global counts

test_count    = 0 # all tests.
skip_count    = 0 # tests that failed to read case.
ignore_count  = 0 # tests that specified ignore.
fail_count    = 0 # tests that ran but failed.


def try_case(path):
  global test_count, skip_count, ignore_count, fail_count
  test_count += 1
  try:
    case = read_case(path)
  except Exception as e:
    errFL('ERROR: could not read test case: {};\nexception: {}', path, e)
    skip_count += 1
    if dbg:
      raise
    else:
      return
  if case.get('ignore'):
    errSL('ignoring: ', path)
    ignore_count += 1
    return
  try:
    ok = run_case(path, case)
  except Exception as e:
    errFL('ERROR: could not run test case: {};\nexception: {}', path, e)
    if dbg:
      raise
    else:
      ok = False
  if not ok:
    fail_count += 1
    outL('=' * bar_width + '\n');
    if fail_fast:
      errFL('exiting fast.')
      sys.exit(1)


# setup
if not _path.exists(results_dir):
  _path.makedirs(results_dir)

# parse and run tests
for path in walk_all_paths(*args.paths, exts=('.test',)):
  if path == '<stdin>':
    errSL('reading test case from', path)
  try_case(path)
  if dbg: errL()

out('\n' + '#' * bar_width + '\nRESULTS: ')
if not any([ignore_count, skip_count, fail_count]):
  outFL('PASSED {} test{}', test_count, ('' if test_count == 1 else 's'))
  code = 0
else:
  outFL('{} tests; IGNORED {}; SKIPPED {}; FAILED {}', test_count, ignore_count, skip_count, fail_count)
  code = 1

sys.exit(code)

