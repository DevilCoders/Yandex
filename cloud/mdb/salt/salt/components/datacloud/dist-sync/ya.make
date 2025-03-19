OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/raven
    contrib/python/pyaml
)

PY_SRCS(
    NAMESPACE dist_sync
    dist_sync.py
)

END()

RECURSE(
    tests
)
