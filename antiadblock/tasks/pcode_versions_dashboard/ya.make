PY2_PROGRAM(pcode_versions_dashboard)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/numpy
    contrib/python/requests
    library/python/yt
    antiadblock/tasks/tools
)

END()
