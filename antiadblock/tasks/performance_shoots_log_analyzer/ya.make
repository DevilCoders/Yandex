PY3_PROGRAM(performance_shoots_log_analyzer)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/pandas
    contrib/python/numpy
    antiadblock/tasks/tools
)

END()
