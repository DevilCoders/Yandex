PY2_PROGRAM(autoredirect_data)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/retry
    contrib/python/requests
    library/python/tvmauth
    antiadblock/tasks/tools
)

END()
