PY2_PROGRAM()

OWNER(spirit-1984)

PEERDIR(
    contrib/python/PyYAML
    library/python/testing/recipe
    library/python/testing/yatest_common
    library/recipes/docker_compose/lib
)

PY_SRCS(__main__.py)

REQUIREMENTS(ram:13)

END()

RECURSE_FOR_TESTS(
    test
)
