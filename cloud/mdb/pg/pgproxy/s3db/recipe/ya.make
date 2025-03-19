OWNER(
    g:mdb
    g:s3
)

PY3_PROGRAM(s3db_recipe)

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/recipes/postgresql/lib
    cloud/mdb/pg/pgproxy/s3db/recipe/lib
    library/python/testing/recipe
    library/python/testing/yatest_common
)

PY_SRCS(
    MAIN
    recipe.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
