PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mlockdb/recipe/recipe.inc)

DATA(arcadia/cloud/mdb/mlockdb)

PEERDIR(
    contrib/python/psycopg2
)

TEST_SRCS(test_recipe.py)

SIZE(MEDIUM)

END()
