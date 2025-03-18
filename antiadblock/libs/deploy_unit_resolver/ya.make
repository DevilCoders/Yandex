PY23_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    lib.py
)
PEERDIR(
    contrib/python/retry
    infra/yp_service_discovery/python/resolver
    infra/yp_service_discovery/api
)

END()
