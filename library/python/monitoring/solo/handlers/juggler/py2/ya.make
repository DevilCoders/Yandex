OWNER(
    g:solo
)

PY2_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
    solo_handler.py
)

PEERDIR(
    library/python/monitoring/solo/handlers/juggler/base
    contrib/python/juggler_sdk
    contrib/python/requests
    contrib/python/retry
)

END()
