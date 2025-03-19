PY3TEST()

OWNER(g:cloud-nbs)

USE_RECIPE(cloud/blockstore/tests/recipes/local-null/local-null-recipe)

DEPENDS(
    cloud/blockstore/daemon
    cloud/blockstore/tests/recipes/local-null
    cloud/blockstore/tools/http_proxy
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
)

PEERDIR(
    kikimr/ci/libraries

    cloud/blockstore/tests/python/lib
)

SIZE(MEDIUM)

TEST_SRCS(
    test.py
)

END()
