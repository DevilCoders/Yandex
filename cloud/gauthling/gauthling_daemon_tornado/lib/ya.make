PY2_LIBRARY(gauthling_daemon_tornado)

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/gauthling_daemon/lib
    cloud/gauthling/gauthling_daemon/proto
    contrib/python/six
    contrib/python/tornado/tornado-4
)

PY_SRCS(
    NAMESPACE gauthling_daemon_tornado
    __init__.py
)

END()
