
OWNER(g:yatest)

PY2TEST()

TEST_SRCS(
    test_behave.py
)

SIZE(LARGE)

TAG(
    ya:fat
    ya:force_sandbox
    ya:nofuse
    ya:dirty
    network:full
)


DATA(
    arcadia/devtools/dummy_arcadia/test/behave
)

PEERDIR(
    devtools/ya/test/tests/lib
)

END()
