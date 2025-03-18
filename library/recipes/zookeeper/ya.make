PY3_PROGRAM(zookeeper)

OWNER(g:metrika-core)

PY_SRCS(__main__.py)

PEERDIR(
    library/python/testing/recipe
    library/recipes/zookeeper/lib
)

END()
