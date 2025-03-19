PY3TEST()

OWNER(g:cloud-nbs)

TEST_SRCS(test.py)

PEERDIR(
    cloud/blockstore/public/sdk/python/client
)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/recipes/local-kikimr/local-kikimr-stable.inc)

SIZE(MEDIUM)

END()
