PY3TEST()

OWNER(g:cloud-nbs)

SIZE(SMALL)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/blockstore/tools/ops/update_limits
    cloud/blockstore/tools/testing/pssh-mock
)

END()
