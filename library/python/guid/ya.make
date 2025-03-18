PY23_LIBRARY()

OWNER(pg)

PEERDIR(
    contrib/python/future
)

PY_SRCS(
    __guid.pyx
    __init__.py
)

END()
