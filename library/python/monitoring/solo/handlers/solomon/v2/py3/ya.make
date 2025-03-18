OWNER(
    g:solo
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
)

PEERDIR(
    library/python/monitoring/solo/handlers/solomon/v2/base
    library/python/monitoring/solo/objects/solomon/v2
    contrib/python/aiohttp
    contrib/python/aiohttp-retry
)

END()
