OWNER(g:mdb)

PY3_PROGRAM(metadb_testdata_recipe)

STYLE_PYTHON()

PEERDIR(
    contrib/python/psycopg2
    library/python/testing/recipe
    library/python/testing/yatest_common
    cloud/mdb/dbaas-internal-api-image/func_tests/util
    cloud/mdb/dbaas_metadb/tests/helpers
)

PY_SRCS(
    MAIN
    recipe.py
)

NO_CHECK_IMPORTS(
    behave.*
    ordereddict
)

END()

RECURSE(
    tests
)
