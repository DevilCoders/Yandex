OWNER(
    g:solo
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
    solo_handler.py
)

PEERDIR(
    library/python/monitoring/solo/handlers/juggler/base
    contrib/python/juggler_sdk
)

END()
