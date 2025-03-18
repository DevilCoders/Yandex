PY2TEST()

OWNER(g:yatool dmitko)

TEST_SRCS(test.py)

# To use docker-compose.yml from another directory, set DOCKER_COMPOSE_FILE variable with Arcadia relative path to the file
# and do not forget to add the directory to the DATA macro, e.g.:
# SET(DOCKER_COMPOSE_FILE library/recipes/docker_compose/test/docker-compose-1.yml)
# DATA(arcadia/library/recipes/docker_compose/test)

SET(DOCKER_CONTEXT_FILE library/recipes/docker_compose/example_with_context/docker-context.yml)
INCLUDE(${ARCADIA_ROOT}/library/recipes/docker_compose/recipe.inc)

SIZE(LARGE)

TAG(
    ya:external
    ya:fat
    ya:force_sandbox
    ya:nofuse
)

REQUIREMENTS(
    # container:716524173 bionic
    container:1435059545 # xenial
)

DATA(
    arcadia/library/recipes/docker_compose/example
)

DEPENDS(devtools/dummy_arcadia/hello_world)

END()
