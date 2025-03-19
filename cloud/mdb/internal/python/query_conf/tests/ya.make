PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

SIZE(SMALL)

PEERDIR(
    cloud/mdb/internal/python/query_conf/tests/sample
)

TEST_SRCS(test_query_conf.py)

END()

RECURSE(
    sample
    mypy
)
