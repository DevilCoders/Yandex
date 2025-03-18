PY2_PROGRAM(forcecry)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/yt
    yql/library/python
    contrib/python/pandas
    library/python/statface_client
    antiadblock/tasks/tools
    antiadblock/tasks/experiments/lib
)

END()
