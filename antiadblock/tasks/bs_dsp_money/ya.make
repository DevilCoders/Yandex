PY3_PROGRAM(bs_dsp_money)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/pandas
    contrib/python/retry
    library/python/statface_client
    antiadblock/tasks/tools
)

END()
