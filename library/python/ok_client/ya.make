PY3_LIBRARY()

PEERDIR(
    contrib/python/requests
    library/python/retry
)

PY_SRCS(
    __init__.py
    client.py
    structure.py
    exception.py
)

END()

RECURSE_FOR_TESTS(
    tests
)

RECURSE(examples)
