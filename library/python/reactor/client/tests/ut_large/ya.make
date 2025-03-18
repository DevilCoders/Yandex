PY23_TEST()

OWNER(g:reactor)

TEST_SRCS(
    __init__.py
#    test_artifact_helpers.py
    test_reaction_helpers.py
#    test_monitorings.py
)

PEERDIR(
    library/python/reactor/client
    library/python/reactor/client/tests/helpers
)


SIZE(LARGE)

TAG(
    ya:fat
    ya:force_sandbox
    ya:external
)

REQUIREMENTS(
    sb_vault:OAUTH_TOKEN=value:REACTOR:REACTOR_ROBOT_OAUTH
    network:full
)

DATA(arcadia/library/python/reactor/client/tests/resourses)

END()
