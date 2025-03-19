PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    z2.py
)

PEERDIR(
    cloud/blockstore/pylibs/common

    contrib/python/requests
)

END()
