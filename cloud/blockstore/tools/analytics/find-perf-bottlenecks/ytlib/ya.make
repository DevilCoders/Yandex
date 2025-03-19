PY23_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    bottlenecks.py
    tracks.py
)

PEERDIR(
    cloud/blockstore/tools/analytics/common
    cloud/blockstore/tools/analytics/find-perf-bottlenecks/lib

    yt/python/client
)

END()
