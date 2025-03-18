PY2_LIBRARY()

OWNER(g:aapi)

PEERDIR(
    aapi/lib/py_helpers
    aapi/lib/py_common
    aapi/lib/proto
    aapi/lib/ydb
    mapreduce/yt/unwrapper
    contrib/python/mercurial
    contrib/python/futures
)

PY_SRCS(
    TOP_LEVEL

    hgext3rd/aapi-convert/__init__.py
    hgext3rd/aapi-convert/convert.py
    hgext3rd/aapi-convert/store.py
    hgext3rd/aapi-convert/upload.py
)

END()

RECURSE(robot)
