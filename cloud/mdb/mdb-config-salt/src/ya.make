OWNER(g:mdb)

PY23_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    TOP_LEVEL
    mdb_salt_returner.py
)

PEERDIR(
    contrib/python/requests
)

END()

RECURSE(
    tests
)
