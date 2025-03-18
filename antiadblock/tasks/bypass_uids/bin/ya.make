PY3_PROGRAM(ANTIADBLOCK_BYPASS_UIDS_TOOL)

OWNER(g:antiadblock)

PY_SRCS(
    MAIN __main__.py
)

PEERDIR(
    contrib/python/click
    contrib/python/pandas

    library/python/yt

    antiadblock/tasks/tools
    antiadblock/tasks/bypass_uids/lib
)

END()
