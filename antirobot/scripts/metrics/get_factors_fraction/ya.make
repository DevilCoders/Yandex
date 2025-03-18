PY3_PROGRAM()

OWNER(
    g:antirobot
)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    contrib/python/click
    infra/yasm/yasmapi
)

END()
