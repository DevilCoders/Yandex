OWNER(
    g:solo
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
)

PEERDIR(
    library/python/monitoring/solo/handlers/solomon/v3/base
    library/python/monitoring/solo/objects/solomon/v3
    contrib/python/aiohttp
    contrib/python/aiohttp-retry
    contrib/python/protobuf
)

END()
