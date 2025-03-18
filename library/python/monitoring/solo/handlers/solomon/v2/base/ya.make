OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
)

PEERDIR(
    library/python/monitoring/solo/objects/solomon/v2
    library/python/monitoring/solo/util
)

END()
