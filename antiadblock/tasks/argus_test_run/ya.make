PY3_PROGRAM(argus_test_run)

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
