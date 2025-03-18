PY2TEST()

OWNER(
    g:yatest
    dmitko
)

TEST_SRCS(test_gtest.py)

SIZE(LARGE)

TAG(
    ya:external
    ya:fat
)

REQUIREMENTS(network:full)

PEERDIR(
    devtools/ya/test/tests/lib
)

DEPENDS(
    devtools/ya/bin
    devtools/ya/test/programs/test_tool/bin
    devtools/ymake/bin
)

END()
