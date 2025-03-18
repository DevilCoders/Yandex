PY23_LIBRARY()

OWNER(
    g:yatest
    dmitko
)

PEERDIR(
    contrib/python/behave
)

PY_SRCS(conftest.py)

END()

RECURSE_FOR_TESTS(
    example
    test
)
