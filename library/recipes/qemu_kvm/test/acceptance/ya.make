OWNER(g:yatool)

PY2TEST()

TEST_SRCS(
    test_run_in_qemu.py
)

SIZE(LARGE)

TAG(
    ya:external
    ya:fat
    ya:force_sandbox
    ya:nofuse
    ya:sys_info
    network:full
)

REQUIREMENTS(
    cpu:all
)

DATA(
   arcadia/library/recipes/qemu_kvm/example
)

PEERDIR(
    devtools/ya/test/tests/lib
)

DEPENDS(
    devtools/ya/bin
    devtools/ya/test/programs/test_tool/bin
    devtools/ymake/bin
)

END()
