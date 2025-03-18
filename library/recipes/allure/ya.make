PY2_PROGRAM(allure_recipe)

OWNER(g:yatool dmitko)

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
    library/recipes/allure/lib
)

PY_SRCS(__main__.py)

END()

RECURSE_FOR_TESTS(
    test
    example/pytest
    example/pytest_fork
)
