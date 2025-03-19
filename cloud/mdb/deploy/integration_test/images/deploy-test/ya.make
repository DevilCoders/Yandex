GO_TEST()

OWNER(g:mdb)

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
    cloud/mdb/deploy/integration_test/images/deploy-test/docker-compose.yml
)

SET(
    RECIPE_CONFIG_FILE
    cloud/mdb/deploy/integration_test/images/deploy-test/recipe-config.yml
)

INCLUDE(${ARCADIA_ROOT}/library/recipes/docker_compose/recipe.inc)

DATA(arcadia/cloud/mdb/deploy)

DATA(arcadia/cloud/mdb/deploydb)

DATA(arcadia/cloud/mdb/mdb-config-salt)

DATA(arcadia/cloud/mdb/salt/salt/_modules)

# https://nda.ya.ru/t/TTgTi8ae4DZm6i

REQUIREMENTS(
    container:1435059545
    cpu:all
    network:full
    dns:dns64
)

DEPENDS(cloud/mdb/deploy/api/cmd/mdb-deploy-api)

DEPENDS(cloud/mdb/deploy/saltkeys/cmd/mdb-deploy-saltkeys)

GO_XTEST_SRCS(
    auth_test.go
    deploy_test.go
)

END()
