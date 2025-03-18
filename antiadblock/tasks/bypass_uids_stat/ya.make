PY3_PROGRAM(bypass_uids_stat)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/yt
    yql/library/python
    contrib/python/pandas
    antiadblock/tasks/tools
)

END()
