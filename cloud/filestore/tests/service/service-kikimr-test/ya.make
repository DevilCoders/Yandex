PY2TEST()

OWNER(g:cloud-nbs)

SIZE(MEDIUM)
TIMEOUT(600)

PEERDIR(
    cloud/filestore/public/sdk/python/client
)

TEST_SRCS(
    test.py
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/tests/recipes/service-kikimr.inc)

END()
