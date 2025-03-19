PY3TEST()

TAG(ya:fat)

SIZE(LARGE)

OWNER(g:cloud-nbs)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/blockstore/support/CLOUDINC-1800/tools/copy_dev
)

TIMEOUT(3600)

PEERDIR(
    cloud/blockstore/tests/python/lib
)

END()
