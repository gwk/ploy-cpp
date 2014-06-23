# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

# global variables

import sys
import gloss_fork.path as _path

# return the base name the path used to invoke the program (argv[0])
process_name = _path.base_name(sys.argv[0])

args = None # populated by gloss.arg.parse()
