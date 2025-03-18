
OWNER(g:yatool)

PY2TEST()

TEST_SRCS(
    test_docker_compose.py
)

SIZE(LARGE)

TAG(
    ya:external
    ya:fat
    ya:force_sandbox
    ya:nofuse
    ya:dirty
    network:full
)

REQUIREMENTS(
    # container:716524173 bionic
    container:799092394 # xenial
    cpu:all dns:dns64
)

DATA(
   arcadia/library/recipes/docker_compose/example
   arcadia/library/recipes/docker_compose/example_with_context
   arcadia/library/recipes/docker_compose/example_test_container
   arcadia/library/recipes/docker_compose/example_with_recipe_config
)

PEERDIR(
    devtools/ya/test/tests/lib
)

DEPENDS(
    devtools/ya/bin
    devtools/ya/test/programs/test_tool/bin
    devtools/ymake/bin
)

END()
