PY3TEST()

OWNER(g:cloud-nbs)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/loadtest/ya.make.inc)

TEST_SRCS(
    test.py
)

USE_RECIPE(cloud/blockstore/tests/recipes/local-null/local-null-recipe)

DEPENDS(
    cloud/blockstore/tests/recipes/local-null
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
    arcadia/cloud/blockstore/tests/loadtest/local-null
)

END()
