PY23_LIBRARY()

OWNER(g:metrika-core)

PY_SRCS(__init__.py)

PEERDIR(
    contrib/python/kazoo
    library/python/testing/recipe
    library/python/testing/yatest_common
    library/recipes/common
)

END()
