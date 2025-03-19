PY3TEST()

TAG(ya:fat)

SIZE(LARGE)

OWNER(g:cloud-nbs)

TEST_SRCS(
    test_with_client.py
    keepalive.py
)

DEPENDS(
    cloud/blockstore/client
    cloud/blockstore/daemon
    cloud/vm/blockstore

    kikimr/driver
)

DATA(
    arcadia/cloud/blockstore/tests/client
)

TIMEOUT(3600)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/tests/python/lib

    kikimr/ci/libraries
)

END()
