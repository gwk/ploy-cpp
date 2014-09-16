# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
[[ $(dirname "$0") == sh ]] || { echo "error: must run from root dir" 1>&2; exit 1; }

make -s _bld/ploy-dbg

#export MallocLogFile=<f> # create/append messages to file <f> instead of stderr.
export MallocGuardEdges=1 # add 2 guard pages for each large block.
#export MallocDoNotProtectPrelude=1 # disable protection (when previous flag set).
#export MallocDoNotProtectPostlude=1 # disable protection (when previous flag set).
#export MallocStackLogging=1 # record all stacks.  Tools like leaks can then be applied.
#export MallocStackLoggingNoCompact=1 # record all stacks.  Needed for malloc_history.
#export MallocStackLoggingDirectory=1 # set location of stack logs, which can grow large; default is /tmp.
export MallocScribble=1 # 0x55 is written upon free and 0xaa is written on allocation.
export MallocCheckHeapStart=1024 # start checking the heap after <n> operations.
export MallocCheckHeapEach=1024 # repeat the checking of the heap after <s> operations.
#export MallocCheckHeapSleep=<t> # sleep <t> seconds on heap corruption.
export MallocCheckHeapAbort=1 # abort on heap corruption if <b> is non-zero.
#export MallocCorruptionAbort=1 # 64-bit: always on; 32-bit: abort on malloc errors, but not on out of memory.
export MallocErrorAbort=1 # abort on any malloc error, including out of memory
#export MallocHelp=1 # this help.

_bld/ploy-dbg "$@"
