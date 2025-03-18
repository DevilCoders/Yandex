PY2_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    lib/dc.py
    lib/__init__.py
    lib/sentinel.py
    lib/test_sentinel.py
)

PEERDIR(
    contrib/python/toredis
    contrib/python/tornado/tornado-4
    contrib/python/enum34
)

END()

RECURSE_FOR_TESTS (
    tests
)
