PY2_PROGRAM(service)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/yt
    antiadblock/analytics_service/service/modules
)

END()
