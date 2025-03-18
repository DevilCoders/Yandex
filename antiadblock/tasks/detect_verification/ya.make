PY3_PROGRAM(detect_verification)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
    config.py
    uids.py
)

PEERDIR(
    library/python/yt
    yql/library/python
    contrib/python/pandas
    antiadblock/tasks/tools
)

END()
