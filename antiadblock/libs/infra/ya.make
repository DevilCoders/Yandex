PY23_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    lib/__init__.py
    lib/infra_client.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/retry
)

END()
