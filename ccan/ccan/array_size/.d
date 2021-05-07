array_size_ok_deps := ccan/array_size/test/compile_fail.c ccan/array_size/test/compile_fail-function-param.c ccan/array_size/test/run.c   ccan/build_assert/.ok 
ccan/array_size/.ok: $(array_size_ok_deps)
ccan/array_size/.fast-ok: $(array_size_ok_deps:%.ok=%.fast-ok)
