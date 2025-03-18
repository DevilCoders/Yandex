PY23_LIBRARY()

OWNER(g:crypta)

PEERDIR(
    contrib/python/six
    contrib/python/requests
)

PY_SRCS(
    __init__.py
    client.py
)


END()
