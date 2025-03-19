OWNER(g:mdb-dataproc)

RECURSE(
    conf
    pillar
    salt
)

PY3TEST()

STYLE_PYTHON()

SIZE(SMALL)

PY_SRCS(
    dataproc_init_actions.py
)

TEST_SRCS(
    tests/test_dataproc_init_actions.py
)

PEERDIR(
    contrib/python/retrying
    contrib/python/responses
    contrib/python/pytest
)

END()

