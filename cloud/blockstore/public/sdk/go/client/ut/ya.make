GO_TEST_FOR(cloud/blockstore/public/sdk/go/client)

OWNER(g:cloud-nbs)

USE_RECIPE(cloud/blockstore/tests/recipes/local-null/local-null-recipe)

SIZE(MEDIUM)
TIMEOUT(600)

DEPENDS(
    cloud/blockstore/daemon
    cloud/blockstore/tests/recipes/local-null
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
)

END()
