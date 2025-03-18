PY3_PROGRAM()

OWNER(
    g:antirobot
)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    contrib/python/numpy
    contrib/python/retry
    infra/yasm/yasmapi
    library/python/solomon
    contrib/python/click
)

END()
