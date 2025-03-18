PY2_PROGRAM(startrek_duty)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/statface_client
    library/python/startrek_python_client
    antiadblock/tasks/tools
)

END()
