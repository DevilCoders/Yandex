PY2_PROGRAM()

OWNER(agorodilov)

PY_SRCS(
    __main__.py
)

PEERDIR(
    kikimr/public/sdk/python/persqueue
    contrib/python/tornado/tornado-4
)

END()
