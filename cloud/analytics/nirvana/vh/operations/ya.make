OWNER(g:cloud_analytics)

PY23_LIBRARY()

PEERDIR(yt/python/client)
PEERDIR(nirvana/valhalla/src)
PEERDIR(cloud/analytics/nirvana/vh/types)

PY_SRCS(
    __init__.py
)

END()
