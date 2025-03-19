PY3TEST()

STYLE_PYTHON()

OWNER(
    g:mdb
    g:s3
)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/pg/pgproxy/s3db/recipe/recipe.inc)

PEERDIR(
    cloud/mdb/recipes/postgresql/lib
    cloud/mdb/pg/pgproxy/s3db/recipe/lib
    contrib/python/psycopg2
    contrib/python/PyHamcrest
)

TEST_SRCS(test_recipe.py)

SIZE(MEDIUM)

END()
