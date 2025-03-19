OWNER(g:mdb)

PY3_PROGRAM(passport-recipe)

STYLE_PYTHON()

PEERDIR(
    contrib/python/Flask
    contrib/python/flask-appconfig
    contrib/python/webargs
    library/python/testing/recipe
    cloud/mdb/dbm/tests/recipes/passport/src
)

ALL_PY_SRCS()

END()

RECURSE_FOR_TESTS(
    tests
)
