OWNER(g:mdb)

PY3_PROGRAM(dbm-recipe)

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/dbm/internal
    library/python/testing/yatest_common
    library/python/testing/recipe
    contrib/python/PyYAML
)

ALL_PY_SRCS()

END()

RECURSE_FOR_TESTS(
    tests
)
