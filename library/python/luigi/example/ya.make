PY3_PROGRAM()

OWNER(pg)

PEERDIR(
    contrib/python/luigi
    library/python/init_log
)

PY_SRCS(
    MAIN main.py
)

END()
