.PHONY: all clean preprocess cg-view test-dbg test-rel test

all: _bld/ploy _bld/ploy-dbg

clean:
	rm -rf _bld/*

_bld/prof-res-usage: tools/cc.sh tools/prof-res-usage.c
	tools/cc.sh tools/prof-res-usage.c -o $@

_bld/text-to-c-literal: tools/cc.sh tools/text-to-c-literal.c
	tools/cc.sh tools/text-to-c-literal.c -o $@

_bld/ploy-core.h: _bld/text-to-c-literal src-core/*.ploy
	cat src-core/*.ploy | _bld/text-to-c-literal 'core_src' > $@

_bld/ploy: tools/cc.sh _bld/ploy-core.h src-boot/*
	tools/cc.sh src-boot/ploy.c -o $@

_bld/ploy-dbg: tools/cc.sh _bld/ploy-core.h src-boot/*
	tools/cc.sh -dbg src-boot/ploy.c -o $@

_bld/ploy-cov: tools/cc.sh _bld/ploy-core.h src-boot/*
	tools/cc.sh -dbg src-boot/ploy.c --coverage -o $@
	mv ploy.gcno _bld/ploy-cov.gcno

# note: ploy.gcda will accumulate results, so it must not exist when the tests run.
_bld/ploy-cov.gcda: tools/run-tests.sh _bld/ploy-cov
	test '!' -r ploy.gcda
	tools/run-tests.sh -cov test/[0-2]-*
	mv ploy.gcda $@

_bld/ploy-cov.llvm-cov: _bld/ploy-cov.gcda
	# coming in llvm 3.5: --all-blocks --branch-probabilities --function-summaries
	llvm-cov -gcno=_bld/ploy-cov.gcno -gcda=_bld/ploy-cov.gcda -o $@

# preprocess for viewing macro expansions.
_bld/ploy-post-proc-no-libs.c: tools/cc.sh _bld/ploy-core.h src-boot/* 
	tools/cc.sh src-boot/ploy.c -o $@ -E -D=SKIP_LIB_INCLUDES

# preprocess for viewing macro expansions.
_bld/ploy-dbg-post-proc-no-libs.c: tools/cc.sh _bld/ploy-core.h src-boot/*
	tools/cc.sh -dbg src-boot/ploy.c -o $@ -E -D=SKIP_LIB_INCLUDES

_bld/ploy.llvm: tools/cc.sh _bld/ploy-core.h src-boot/* 
	tools/cc.sh src-boot/ploy.c -o $@ -S -emit-llvm

_bld/ploy-dbg.llvm: tools/cc.sh _bld/ploy-core.h src-boot/* 
	tools/cc.sh -dbg src-boot/ploy.c -o $@ -S -emit-llvm

_bld/compile_commands.json: tools/cdb.sh tools/cc.sh
	$^ $@

_bld/ploy-callgraph.txt: tools/gen-callgraph-txt.sh _bld/ploy-dbg.llvm
	$^ $@

_bld/ploy-call-sccs.txt: tools/gen-call-sccs-txt.sh _bld/ploy-dbg.llvm
	$^ $@

_bld/ploy-callgraph.dot: tools/gen-callgraph-dot.py _bld/ploy-callgraph.txt _bld/ploy-call-sccs.txt
	$^ $@

_bld/ploy-callgraph.svg: tools/gen-callgraph-svg.sh _bld/ploy-callgraph.dot
	$^ $@

_bld/ploy-ast-list.txt: _bld/compile_commands.json _bld/ploy-core.h src-boot/*
	clang-check -p _bld/compile_commands.json src-boot/ploy.c -ast-list > $@

_bld/ploy-ast-print.txt: _bld/compile_commands.json _bld/ploy-core.h src-boot/*
	clang-check -p _bld/compile_commands.json src-boot/ploy.c -ast-print > $@

_bld/ploy-ast-dump.txt: _bld/compile_commands.json _bld/ploy-core.h src-boot/*
	clang-check -p _bld/compile_commands.json src-boot/ploy.c -ast-dump > $@

preprocess: _bld/ploy-post-proc-no-libs.c _bld/ploy-dbg-post-proc-no-libs.c

ast: _bld/ploy-ast-list.txt _bld/ploy-ast-print.txt _bld/ploy-ast-dump.txt

# output the useless plist to /dev/null.
analyze: tools/cc.sh src-boot/*
	tools/cc.sh -dbg src-boot/ploy.c --analyze -o /dev/null

callgraph: _bld/ploy-callgraph.svg
	open -a Safari _bld/ploy-callgraph.svg

# omit perf tests, which take too long in debug mode.
test-dbg: tools/run-tests.sh _bld/ploy-dbg
	@echo "\ntest-dbg:"
	tools/run-tests.sh -dbg test/[0-2]-*

test-rel: tools/run-tests.sh _bld/ploy
	@echo "\ntest-rel:"
	tools/run-tests.sh test

test: test-dbg test-rel

perf-test: _bld/prof-res-usage
	tools/perf-test-all.sh
