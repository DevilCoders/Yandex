OWNER(g:mdb)

PY2TEST()

PEERDIR(
    library/python/testing/behave
    contrib/python/docker
    contrib/python/psycopg2
)

TAG(
    ya:fat
    ya:force_sandbox
    ya:noretries
    ya:nofuse
)

TIMEOUT(1800)

SIZE(LARGE)

SET(
    DOCKER_COMPOSE_FILE
    cloud/mdb/mdb-pgcheck/docker-compose.yml
)

SET(
    RECIPE_CONFIG_FILE
    cloud/mdb/mdb-pgcheck/recipe-config.yml
)

INCLUDE(${ARCADIA_ROOT}/library/recipes/docker_compose/recipe.inc)

DATA(arcadia/cloud/mdb/mdb-pgcheck)

DEPENDS(cloud/mdb/mdb-pgcheck/cmd/mdb-pgcheck)

REQUIREMENTS(
    container:716524173
    cpu:all
    network:full
    dns:dns64
)

NO_CHECK_IMPORTS()

NO_LINT()

END()

RECURSE(
    cmd
    yo_test
)
