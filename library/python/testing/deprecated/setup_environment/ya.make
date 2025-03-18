PY23_LIBRARY()

OWNER(g:yatest)

PY_SRCS(__init__.py)

PEERDIR(
    library/python/testing/yatest_common
)

END()

RECURSE(
    recipe
)
