PY3_PROGRAM(maas)

OWNER(g:cloud-ps)

PEERDIR(
    contrib/python/requests
)

PY_SRCS(
    TOP_LEVEL
    MAIN run.py

    clients/__init__.py
    clients/instances.py
    supdater/__init__.py
    supdater/graphs.py
)

END()