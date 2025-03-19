PY2TEST()

SIZE(MEDIUM)

OWNER(g:cloud-nbs)

TEST_SRCS(
    test.py
)

TIMEOUT(360)

DEPENDS(
    cloud/blockstore/tools/testing/eternal-tests/eternal-load/bin
)

PEERDIR(
    contrib/python/futures
)

END()
