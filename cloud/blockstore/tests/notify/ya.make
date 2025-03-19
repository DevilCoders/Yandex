PY3TEST()

SIZE(MEDIUM)

OWNER(g:cloud-nbs)

TEST_SRCS(test.py)

DEPENDS(
    cloud/blockstore/client
    cloud/blockstore/daemon
    cloud/blockstore/tools/testing/notify-mock
    kikimr/driver
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/tests/python/lib
    contrib/python/requests
    kikimr/ci/libraries
    ydb/core/protos
)

END()
