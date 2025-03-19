PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    errors.py
    z2.py
)

PEERDIR(
    cloud/blockstore/pylibs/clients/z2
    cloud/blockstore/pylibs/clusters
)

END()
