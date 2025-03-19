PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql-replica/recipe.inc)

PEERDIR(
    contrib/python/psycopg2
    cloud/mdb/recipes/postgresql/lib
)

TEST_SRCS(test_replication.py)

SIZE(MEDIUM)

END()
