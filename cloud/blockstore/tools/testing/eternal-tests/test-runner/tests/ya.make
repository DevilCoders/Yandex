PY3TEST()

OWNER(g:cloud-nbs)

SIZE(MEDIUM)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/blockstore/tools/testing/eternal-tests/test-runner
)

END()
