PY23_TEST()

OWNER(g:cloud-nbs)

SIZE(MEDIUM)
TIMEOUT(600)

PY_SRCS(
    client_methods.py
)

TEST_SRCS(
    client_ut.py
    durable_ut.py
    grpc_client_ut.py
    http_client_ut.py
)

PEERDIR(
    cloud/filestore/public/api/grpc
    cloud/filestore/public/sdk/python/client
    cloud/filestore/public/sdk/python/protos

    library/python/testing/yatest_common
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-null.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/vhost-null.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/http-proxy.inc)

END()
