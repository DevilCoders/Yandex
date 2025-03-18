OWNER(
    g:solo
)

PY2_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
)

PEERDIR(
    library/python/monitoring/solo/handlers/solomon/v3/base
    library/python/monitoring/solo/objects/solomon/v3
    contrib/python/requests
    contrib/python/retry
    contrib/python/protobuf
)

END()
