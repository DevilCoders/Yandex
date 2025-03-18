OWNER(
    g:solo
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
)

PEERDIR(
    library/python/monitoring/solo/handlers/yasm/base
    contrib/python/aiohttp
    contrib/python/aiohttp-retry
)

END()
