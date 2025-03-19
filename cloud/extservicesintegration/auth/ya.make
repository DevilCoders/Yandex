PY2_PROGRAM()

OWNER(agorodilov)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/gauthling/yc_auth/lib
    contrib/python/tornado/tornado-4
    contrib/python/cachetools
)

END()
