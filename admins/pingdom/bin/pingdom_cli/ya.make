OWNER(g:kp-sre)

PY3_PROGRAM(pingdom_cli)

PY_SRCS(
    MAIN __init__.py
)

PEERDIR(
    contrib/python/requests
)

END()
