PY3TEST()

OWNER(g:mstand)

SIZE(SMALL)

TEST_SRCS(
    tools/mstand/criterias/auto_ut.py
)

PEERDIR(
    tools/mstand/criterias
    tools/mstand/experiment_pool
    tools/mstand/serp
    tools/mstand/session_squeezer
)

DATA(
    arcadia/tools/mstand/criterias/tests/ut/data
)

END()
