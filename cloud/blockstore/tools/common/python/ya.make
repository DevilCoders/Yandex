PY2_LIBRARY()

OWNER(g:cloud-nbs)

PEERDIR(
    contrib/python/requests
)

PY_SRCS(NAMESPACE cloud.blockstore.tools
    __init__.py
    kikimrapi.py
    nbsapi.py
    solomonapi.py
    util.py
)

END()
