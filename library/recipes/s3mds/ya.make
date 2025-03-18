OWNER(
    g:maps-dragon-fighters
    g:s3
)

PY3_PROGRAM(s3mds-recipe)

PEERDIR(
    contrib/python/requests
    library/python/testing/recipe
    library/python/testing/yatest_common
    library/recipes/common
)

PY_SRCS(
    MAIN main.py
)

END()

RECURSE(tests)

