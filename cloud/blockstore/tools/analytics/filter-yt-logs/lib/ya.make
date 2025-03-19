PY23_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
)

PEERDIR(
    cloud/blockstore/tools/analytics/common

    yt/python/client
)

END()
