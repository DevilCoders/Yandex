PY3TEST()

OWNER(g:cloud-nbs)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/loadtest/ya.make.inc)

TAG(
    ya:external
    ya:not_autocheck
)

TEST_SRCS(
    test.py
)

DATA(
    arcadia/cloud/blockstore/tests/loadtest/remote
)

END()
