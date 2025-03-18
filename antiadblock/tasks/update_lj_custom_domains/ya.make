PY2_PROGRAM(update_lj_custom_domains)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/pyre2
    contrib/python/retry
    contrib/python/requests
    library/python/tvmauth
    antiadblock/tasks/tools
)

END()
