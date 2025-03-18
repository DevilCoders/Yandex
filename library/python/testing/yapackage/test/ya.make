PY2TEST()

OWNER(
    g:yatest
    dmitko
)

TEST_SRCS(test_yapackage.py)

PEERDIR(
    library/python/testing/yapackage
)

DATA(arcadia/devtools/ya/package/tests/fat/data)

TAG(
    ya:dirty
    ya:fat
    ya:force_sandbox
)

SIZE(LARGE)

END()
