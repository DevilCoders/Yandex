PY3TEST()

SIZE(MEDIUM)
TIMEOUT(600)

OWNER(g:cloud-nbs)

TEST_SRCS(
    test.py
)

DEPENDS(
)

PEERDIR(
    cloud/blockstore/public/sdk/python/client
)

END()
