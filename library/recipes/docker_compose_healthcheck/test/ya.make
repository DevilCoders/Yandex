PY2TEST()

OWNER(spirit-1984)

TEST_SRCS(test.py)

# To use docker-compose.yml from another directory, set DOCKER_COMPOSE_FILE variable with Arcadia relative path to the file
# and do not forget to add the directory to the DATA macro, e.g.:
# SET(DOCKER_COMPOSE_FILE library/recipes/docker_compose/test/docker-compose-1.yml)
# DATA(arcadia/library/recipes/docker_compose/test)

INCLUDE(${ARCADIA_ROOT}/library/recipes/docker_compose_healthcheck/recipe.inc)

SIZE(LARGE)

TAG(
    ya:external
    ya:fat
    ya:force_sandbox
    ya:nofuse
)

REQUIREMENTS(
    # container:716524173 bionic
    container:799092394 # xenial
    cpu:all dns:dns64
)

END()
