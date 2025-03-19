PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    helpers.py
)

PEERDIR(
    cloud/blockstore/pylibs/clusters
    cloud/blockstore/pylibs/ycp
)

END()
