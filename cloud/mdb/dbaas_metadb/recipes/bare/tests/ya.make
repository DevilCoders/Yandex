PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

PEERDIR(
    contrib/python/psycopg2
)

TEST_SRCS(test_recipe.py)

REQUIREMENTS(network:full)

SIZE(MEDIUM)

END()
