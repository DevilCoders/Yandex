PY23_LIBRARY()

OWNER(
    g:python-contrib
    dshmatkov
)

PEERDIR(
    devtools/swag/lib
    contrib/python/retry
)

PY_SRCS(
    TOP_LEVEL
    pytest_mongodb/__init__.py
    pytest_mongodb/config.py
    pytest_mongodb/conftest.py
    pytest_mongodb/server.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
