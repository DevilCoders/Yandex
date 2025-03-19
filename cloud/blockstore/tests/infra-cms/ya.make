PY3TEST()

TAG(ya:fat)

SIZE(LARGE)

OWNER(g:cloud-nbs)

TEST_SRCS(test.py)

DEPENDS(
    cloud/blockstore/client
    cloud/blockstore/daemon
    cloud/blockstore/tools/http_proxy
    cloud/blockstore/tools/testing/loadtest/bin
    cloud/storage/core/tools/testing/unstable-process
    kikimr/driver
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
    arcadia/cloud/blockstore/tests/loadtest/local-nonrepl
)

TIMEOUT(3600)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/tests/python/lib
    kikimr/ci/libraries
    ydb/core/protos
)

END()
