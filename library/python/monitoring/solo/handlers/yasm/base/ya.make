OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    handler.py
)

PEERDIR(
    library/python/monitoring/solo/objects/yasm
    library/python/monitoring/solo/util
)

END()
