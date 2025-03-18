PY23_LIBRARY()

OWNER(g:ci)

PY_SRCS(
    __init__.py
)

IF(PYTHON2)
    PEERDIR(
        contrib/python/typing
    )
ENDIF()

PEERDIR(
    infra/yp_service_discovery/api
    infra/yp_service_discovery/python/resolver
)

END()

RECURSE(
    tools
)
