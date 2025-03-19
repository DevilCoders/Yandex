PY23_LIBRARY(yc_requests)

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/auth_token_python/lib
    contrib/python/requests
    contrib/python/six
)

IF (PYTHON2)
    PEERDIR(contrib/python/typing)
ENDIF()

PY_SRCS(
    NAMESPACE yc_requests
    __init__.py
    api.py
    credentials.py
    service_names.py
    sessions.py
    signing.py
)

END()
