.PHONY: all clean preprocess cg-view test-dbg test-rel test

all: _bld/ploy _bld/ploy-dbg

clean:
	rm -rf _bld/*

_bld/prof-res-usage: tools/cc.sh tools/prof-res-usage.c
	tools/cc.sh tools/prof-res-usage.c -o $@

_bld/text-to-c-literal: tools/cc.sh tools/text-to-c-literal.c
	tools/cc.sh tools/text-to-c-literal.c -o $@

_bld/ploy-core.h: _bld/text-to-c-literal src/core.ploy
	_bld/text-to-c-literal 'core_src' < src/core.ploy > $@

_bld/ploy: tools/cc.sh _bld/ploy-core.h src/*
	tools/cc.sh src/ploy.c -o $@

_bld/ploy-dbg: tools/cc.sh _bld/ploy-core.h src/*
	tools/cc.sh -dbg src/ploy.c -o $@

# preprocess for viewing macro expansions.
_bld/ploy-post-proc-no-libs.c: tools/cc.sh _bld/ploy-core.h src/* 
	tools/cc.sh src/ploy.c -o $@ -E -D=SKIP_LIB_INCLUDES

# preprocess for viewing macro expansions.
_bld/ploy-dbg-post-proc-no-libs.c: tools/cc.sh _bld/ploy-core.h src/*
	tools/cc.sh -dbg src/ploy.c -o $@ -E -D=SKIP_LIB_INCLUDES

_bld/ploy.llvm: tools/cc.sh _bld/ploy-core.h src/* 
	tools/cc.sh src/ploy.c -o $@ -S -emit-llvm

_bld/ploy-dbg.llvm: tools/cc.sh _bld/ploy-core.h src/* 
	tools/cc.sh -dbg src/ploy.c -o $@ -S -emit-llvm

_bld/compile_commands.json: tools/cc.sh tools/cdb.sh
	tools/cdb.sh

_bld/ploy-callgraph.txt: tools/gen-callgraph-txt.sh _bld/ploy-dbg.llvm
	tools/gen-callgraph-txt.sh -print-callgraph _bld/ploy-dbg.llvm $@

_bld/ploy-call-sccs.txt: tools/gen-callgraph-txt.sh _bld/ploy-dbg.llvm
	tools/gen-callgraph-txt.sh -print-callgraph-sccs _bld/ploy-dbg.llvm $@

_bld/ploy-callgraph.dot: tools/gen-callgraph-dot.py _bld/ploy-callgraph.txt _bld/ploy-call-sccs.txt
	$^ $@

_bld/ploy-callgraph.svg: tools/gen-callgraph-svg.sh _bld/ploy-callgraph.dot
	$^ $@

_bld/ploy-call-scc-names.txt: tools/gen-call-sccs-dot.py _bld/ploy-call-sccs.txt
	$^ $@

_bld/ploy-ast-list.txt: _bld/compile_commands.json src/*
	clang-check -p _bld/compile_commands.json -ast-list src/ploy.c > _bld/ploy-ast-list.txt

_bld/ploy-ast-print.txt: _bld/compile_commands.json src/*
	clang-check -p _bld/compile_commands.json -ast-print src/ploy.c > _bld/ploy-ast-print.txt

preprocess: _bld/ploy-post-proc-no-libs.c _bld/ploy-dbg-post-proc-no-libs.c

# output the useless plist to /dev/null.
analyze: tools/cc.sh src/*
	tools/cc.sh -dbg src/ploy.c --analyze -o /dev/null

callgraph: _bld/ploy-callgraph.svg
	open -a Safari _bld/ploy-callgraph.svg

# omit perf tests, which take too long in debug mode.
test-dbg: tools/run-tests.sh _bld/ploy-dbg
	@echo "\ntest-dbg:"
	tools/run-tests.sh -dbg test/[0-3]-*

test-rel: tools/run-tests.sh _bld/ploy
	@echo "\ntest-rel:"
	tools/run-tests.sh test

test: test-rel test-dbg

perf-test: _bld/prof-res-usage
	tools/perf-test-all.sh
