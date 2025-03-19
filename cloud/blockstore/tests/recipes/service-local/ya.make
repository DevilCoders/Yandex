PY3_PROGRAM(service-local-recipe)

OWNER(g:cloud-nbs)

PY_SRCS(__main__.py)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/tests/python/lib

    library/python/testing/recipe
    library/python/testing/yatest_common

    kikimr/ci/libraries
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
)

END()
