PY3_PROGRAM(push_balancer_crypto_settings)

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
