PY23_LIBRARY()

OWNER(
    dmitko
    korum
)

PEERDIR(
    contrib/python/six
)

PY_SRCS(__init__.py)

END()

RECURSE_FOR_TESTS(
    tests
)
