DEPENDS(
    library/recipes/docker_compose_healthcheck
    library/recipes/docker_compose/bin
)

USE_RECIPE(
    library/recipes/docker_compose_healthcheck/docker_compose_healthcheck --compose-file $DOCKER_COMPOSE_FILE --context-file $DOCKER_CONTEXT_FILE
)

REQUIREMENTS(cpu:all)
