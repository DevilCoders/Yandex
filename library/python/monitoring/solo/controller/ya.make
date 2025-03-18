OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    controller.py
    resource.py
    state.py
)

PEERDIR(
    library/python/monitoring/solo/handlers/solomon
    library/python/monitoring/solo/handlers/juggler
    library/python/monitoring/solo/handlers/yasm
    library/python/monitoring/solo/objects/solomon/v2
    library/python/monitoring/solo/objects/solomon/v3
    library/python/monitoring/solo/objects/juggler
    library/python/monitoring/solo/objects/yasm
    library/python/monitoring/solo/util
    library/python/yt
)

END()

