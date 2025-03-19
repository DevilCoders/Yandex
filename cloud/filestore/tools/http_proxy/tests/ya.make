PY2TEST()

OWNER(g:cloud-nbs)

DEPENDS(
    cloud/filestore/server
    cloud/filestore/tests/recipes/service-local
    cloud/filestore/tools/http_proxy
)

PEERDIR(
    kikimr/ci/libraries

    cloud/filestore/tests/python/lib
)

SIZE(MEDIUM)

TEST_SRCS(
    test.py
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-null.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/vhost.inc)

END()
