PY2_PROGRAM()

OWNER(g:aapi)

PEERDIR(
    aapi/lib/py_common
    aapi/lib/py_helpers
    aapi/lib/proto
    mapreduce/yt/unwrapper
    contrib/python/futures
)

PY_SRCS(
    __main__.py
)

END()
