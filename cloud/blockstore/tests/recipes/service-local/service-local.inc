DEPENDS(
    cloud/blockstore/daemon
    cloud/blockstore/tests/recipes/service-local

    kikimr/driver
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
)

USE_RECIPE(
    cloud/blockstore/tests/recipes/service-local/service-local-recipe
)

