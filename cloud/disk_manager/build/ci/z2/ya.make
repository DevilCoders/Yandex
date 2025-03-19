PY23_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    client.py
)

PEERDIR(
    contrib/python/requests
)

END()
