OWNER(g:mdb)

PY3_PROGRAM(deploy-api-recipe)

STYLE_PYTHON()

PEERDIR(
    contrib/python/Flask
    library/python/testing/recipe
)

ALL_PY_SRCS()

END()

RECURSE_FOR_TESTS(
    tests
)
