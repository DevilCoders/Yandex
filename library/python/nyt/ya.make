PY23_LIBRARY()

OWNER(
    borman
)

PEERDIR(
    mapreduce/yt/client
    library/python/nyt/native
)

PY_SRCS(
    __init__.py
    log.pyx
    client.pyx
)

END()
