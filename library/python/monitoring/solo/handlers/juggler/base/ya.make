OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
    solo_handler.py
)

PEERDIR(
    library/python/monitoring/solo/util
    contrib/python/retry
)

END()
