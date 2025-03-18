PY3_PROGRAM()

OWNER(g:antirobot)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/retry
    library/python/deprecated/ticket_parser2
)

END()
