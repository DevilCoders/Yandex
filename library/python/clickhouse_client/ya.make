PY2_LIBRARY()

OWNER(
    g:statinfra
)

PEERDIR(
    contrib/python/requests
    contrib/python/six
)

PY_SRCS(
    TOP_LEVEL
    clickhouse/__init__.py
    clickhouse/client.py
    clickhouse/errors.py
    clickhouse/tools.py
)

END()

RECURSE_FOR_TESTS(
    tests
)

