PY23_LIBRARY(yc_auth_tornado)

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/yc_auth/lib
    cloud/gauthling/gauthling_daemon/lib
    contrib/python/tornado/tornado-4
)

PY_SRCS(
    NAMESPACE yc_auth_tornado
    __init__.py
    as_client.py
    authentication.py
    authorization.py
    exceptions.py
    futures.py
    scms_agent.py
)

END()
