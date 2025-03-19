PY23_LIBRARY(gauthling_daemon)

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/gauthling_daemon/proto
    contrib/python/six
)

PY_SRCS(
    NAMESPACE gauthling_daemon
    __init__.py
    errors.py
    rpc/__init__.py
)

END()
