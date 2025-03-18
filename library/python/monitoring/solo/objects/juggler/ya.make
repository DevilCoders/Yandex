OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    dashboard.py
    base.py
)

PEERDIR(
    contrib/python/juggler_sdk
    contrib/python/jsonobject
    contrib/python/six
    library/python/monitoring/solo/objects/common
)

END()
