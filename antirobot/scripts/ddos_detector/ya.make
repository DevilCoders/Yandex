PY3_PROGRAM()

OWNER(g:antirobot)

PY_SRCS(
    __main__.py
)

PEERDIR(
     infra/yasm/yasmapi
     contrib/python/requests
     contrib/python/certifi
)

END()
