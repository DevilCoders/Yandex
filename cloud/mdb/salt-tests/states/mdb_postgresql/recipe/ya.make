OWNER(g:mdb)

PY3_PROGRAM(mdb_postgresql_recipe)

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/recipes/postgresql/lib
    library/python/testing/recipe
    library/python/testing/yatest_common
    contrib/python/psycopg2
)

PY_SRCS(
    MAIN
    recipe.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
