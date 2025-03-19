PY3TEST()

SIZE(MEDIUM)

OWNER(g:cloud-nbs)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/blockstore/client
    cloud/blockstore/daemon
    cloud/blockstore/tools/testing/plugintest
    cloud/blockstore/tools/testing/stable-plugin
    cloud/storage/core/tools/testing/unstable-process
    cloud/vm/blockstore

    kikimr/driver
)

DATA(
    arcadia/cloud/blockstore/tests/plugin/tcp
)

TIMEOUT(600)

PEERDIR(
    cloud/blockstore/tests/python/lib

    kikimr/ci/libraries
)

END()
