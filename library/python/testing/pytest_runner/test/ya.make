OWNER(
    g:yatest
    dmitko
)

PY2TEST()

TEST_SRCS(test_pytest_runner.py)

PEERDIR(
    devtools/ya/test/tests/lib
)

DATA(
   arcadia/library/python/testing/pytest_runner
   arcadia/dummy_arcadia/pytests-samples
)

SIZE(LARGE)

TAG(
    ya:dirty
    ya:fat
    ya:force_sandbox
    ya:nofuse
)

REQUIREMENTS(
    container:387074777
    network:full
)

DEPENDS(
    devtools/ya/bin
    devtools/ya/test/programs/test_tool/bin
    devtools/ymake/bin
)

END()
