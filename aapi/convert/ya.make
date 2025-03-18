PY2_PROGRAM()

OWNER(g:aapi)

PEERDIR(
    contrib/python/patched/subvertpy
    contrib/python/futures
    contrib/python/Flask-RESTful
    mapreduce/yt/unwrapper
    aapi/lib/convert
    aapi/lib/proto
    aapi/lib/ydb
)

PY_SRCS(
    __main__.py
    store.py
)

END()
