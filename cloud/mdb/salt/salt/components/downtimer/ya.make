OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/psycopg2
    contrib/python/requests
    contrib/python/raven
)

PY_SRCS(downtimer.py)

END()

RECURSE_FOR_TESTS(
    tests
)
