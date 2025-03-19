PY3TEST()

OWNER(g:cloud-nbs)

TEST_SRCS(test.py)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/tests/python/lib

    library/python/testing/yatest_common

    kikimr/ci/libraries

    contrib/python/requests
)

DEPENDS(
    cloud/blockstore/daemon
    kikimr/driver
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
)

SIZE(MEDIUM)

END()
