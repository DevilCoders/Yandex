PY3_PROGRAM()

OWNER(g:passport_infra)

PY_SRCS(__main__.py)

PEERDIR(
    contrib/python/requests
    library/python/testing/recipe
    library/python/testing/yatest_common
    library/recipes/common
)

END()

RECURSE_FOR_TESTS(
    ut_simple
)
