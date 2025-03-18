OWNER(
    g:solo
)

PY2_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
)

PEERDIR(
    library/python/monitoring/solo/handlers/yasm/base
    contrib/python/retry
)

END()
