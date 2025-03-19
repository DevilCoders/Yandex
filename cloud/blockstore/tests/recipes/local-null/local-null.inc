DEPENDS(
    cloud/blockstore/daemon
    cloud/blockstore/tests/recipes/local-null

    kikimr/driver
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
)

USE_RECIPE(
    cloud/blockstore/tests/recipes/local-null/local-null-recipe
)

