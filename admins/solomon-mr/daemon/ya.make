PY3_PROGRAM(solomon-mr-daemon)

OWNER(music-sre)

PY_SRCS(
    MAIN __main__.py
)

PEERDIR(
    admins/solomon-mr/src
)

END()
