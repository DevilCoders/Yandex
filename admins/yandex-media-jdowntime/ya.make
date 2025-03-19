PY3_PROGRAM(jdowntime)

OWNER(skacheev)

PY_SRCS(
    MAIN src/usr/bin/jdowntime.py
)

PEERDIR(
    contrib/python/requests
)

END()
