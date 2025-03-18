RECURSE(
    v2
    v3
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
)

PEERDIR(
    library/python/monitoring/solo/objects/solomon/v2
    library/python/monitoring/solo/objects/solomon/v3
    library/python/monitoring/solo/handlers/solomon/v2
    library/python/monitoring/solo/handlers/solomon/v3
)

END()
