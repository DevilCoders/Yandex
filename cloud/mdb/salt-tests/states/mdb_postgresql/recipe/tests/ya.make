PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/salt-tests/states/mdb_postgresql/recipe/recipe.inc)

PEERDIR(
    contrib/python/psycopg2
)

TEST_SRCS(test_recipe.py)

REQUIREMENTS(network:full)

SIZE(MEDIUM)

END()
