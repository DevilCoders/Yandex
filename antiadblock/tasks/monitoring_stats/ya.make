PY3_PROGRAM(monitoring_stats)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/retry
    library/python/startrek_python_client
    library/python/statface_client
    antiadblock/tasks/tools
)

END()
