PY3_PROGRAM(result_agregate_task)

OWNER(g:antiadblock)

PEERDIR(
    sandbox/common/rest
    sandbox/common/types
    sandbox/common
    library/python/tvmauth
    antiadblock/tasks/tools
)

PY_SRCS(
    __main__.py
)

END()
