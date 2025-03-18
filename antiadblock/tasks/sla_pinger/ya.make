PY2_PROGRAM(sla_pinger)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    antiadblock/libs/utils
    library/python/startrek_python_client
    library/python/statface_client
    contrib/python/requests
)

END()
