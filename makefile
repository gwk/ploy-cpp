# $@: The file name of the target of the rule.
# $<: The name of the first prerequisite.
# $^: The names of all the prerequisites, with spaces between them. 

# first target is the default.
default: basic

_bld/prof-res-usage: tools/cc.sh tools/prof-res-usage.cpp
	$^ -o $@

_bld/ploy: tools/cc.sh src-boot/*
	tools/cc.sh src-boot/ploy.cpp -o $@

_bld/ploy-dbg: tools/cc.sh src-boot/*
	tools/cc.sh -dbg src-boot/ploy.cpp -o $@

_bld/ploy-cov: tools/cc.sh src-boot/*
	tools/cc.sh -dbg src-boot/ploy.cpp --coverage -o $@
	# compiling ploy-cov also produces the .gcno file.
	mv ploy.gcno _bld/ploy-cov.gcno

_bld/ploy-cov.gcda: tools/run-tests.sh _bld/ploy-cov
	# ploy.gcda accumulates results, so it must not exist when the tests run.
	rm -f ploy.gcda
	$^ test/[0-2]-*
	mv ploy.gcda $@

_bld/ploy-cov-summary-raw.txt: _bld/ploy-cov _bld/ploy-cov.gcda
	llvm-cov -a -b -f $< > $@
	mv *.gcov _bld/

_bld/ploy-cov-summary.txt: tools/gen-cov-summary.py _bld/ploy-cov-summary-raw.txt
	$^ > $@

# preprocess for viewing macro expansions.
_bld/ploy-post-proc-no-libs.cpp: tools/cc.sh src-boot/* 
	tools/cc.sh src-boot/ploy.cpp -o $@ -E -D=SKIP_LIB_INCLUDES

# preprocess for viewing macro expansions.
_bld/ploy-dbg-post-proc-no-libs.cpp: tools/cc.sh src-boot/*
	tools/cc.sh -dbg src-boot/ploy.cpp -o $@ -E -D=SKIP_LIB_INCLUDES

_bld/ploy.ll: tools/cc.sh src-boot/* 
	tools/cc.sh src-boot/ploy.cpp -o $@ -S -emit-llvm

_bld/ploy-dbg.ll: tools/cc.sh src-boot/* 
	tools/cc.sh -dbg src-boot/ploy.cpp -o $@ -S -emit-llvm

_bld/compile_commands.json: tools/cdb.sh tools/cc.sh
	$^ $@

_bld/ploy-callgraph.txt: tools/gen-callgraph-txt.sh _bld/ploy-dbg.ll
	$^ $@

_bld/ploy-call-sccs.txt: tools/gen-call-sccs-txt.sh _bld/ploy-dbg.ll
	$^ $@

_bld/ploy-callgraph.dot: tools/gen-callgraph-dot.py _bld/ploy-callgraph.txt _bld/ploy-call-sccs.txt
	$^ $@

_bld/ploy-callgraph.svg: tools/gen-callgraph-svg.sh _bld/ploy-callgraph.dot
	$^ $@

_bld/ploy-ast-list.txt: _bld/compile_commands.json src-boot/*
	clang-check -p _bld/compile_commands.json src-boot/ploy.cpp -ast-list > $@

_bld/ploy-ast-print.txt: _bld/compile_commands.json src-boot/*
	clang-check -p _bld/compile_commands.json src-boot/ploy.cpp -ast-print > $@

_bld/ploy-ast-dump.txt: _bld/compile_commands.json src-boot/*
	clang-check -p _bld/compile_commands.json src-boot/ploy.cpp -ast-dump > $@

.PHONY: all basic clean preprocess ast cov ll analyze callgraph test-dbg test-rel test perf-test

all: basic preprocess ast cov ll analyze callgraph test perf-test

basic: _bld/ploy _bld/ploy-dbg

clean:
	rm -rf _bld/*

preprocess: _bld/ploy-post-proc-no-libs.cpp _bld/ploy-dbg-post-proc-no-libs.cpp

ast: _bld/ploy-ast-list.txt _bld/ploy-ast-print.txt _bld/ploy-ast-dump.txt

cov: _bld/ploy-cov-summary.txt
	cat $^

ll: _bld/ploy.ll _bld/ploy-dbg.ll

# output the useless plist to /dev/null.
analyze: tools/cc.sh src-boot/*	
	tools/cc.sh -dbg src-boot/ploy.cpp --analyze -o /dev/null

callgraph: _bld/ploy-callgraph.svg
	open -a Safari _bld/ploy-callgraph.svg

# omit perf tests, which take too long in debug mode.
test-dbg: tools/run-tests.sh _bld/ploy-dbg
	@echo "\ntest-dbg:"
	$^ test/[0-2]-*

test-rel: tools/run-tests.sh _bld/ploy
	@echo "\ntest-rel:"
	$^ test/[0-3]-*

test: test-dbg test-rel

perf-test: _bld/prof-res-usage
	tools/perf-test-all.sh
