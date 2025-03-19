PY23_LIBRARY(yc_auth)

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/auth_token_python/lib
    cloud/gauthling/gauthling_daemon/lib
    cloud/gauthling/yc_auth/proto
    cloud/gauthling/yc_requests/lib
    cloud/iam/accessservice/client/python
    contrib/python/six
)

PY_SRCS(
    NAMESPACE yc_auth
    __init__.py
    authentication.py
    authorization.py
    exceptions.py
    gauthling.py
    scms_agent.py
    utils.py
)

END()
