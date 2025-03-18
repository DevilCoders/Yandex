PY23_LIBRARY()

OWNER(borman)

PEERDIR(
    contrib/python/luigi
    contrib/python/tornado/tornado-4
    library/python/resource
    library/python/luigi/data
)

PY_SRCS(static_server.py)

END()
