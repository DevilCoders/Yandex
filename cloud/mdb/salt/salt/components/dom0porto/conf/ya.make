OWNER(g:mdb)

PY2_LIBRARY()

PEERDIR(
    contrib/python/requests
)


PY_SRCS(
    heartbeat.py
)

END()

RECURSE(
    tests
)
