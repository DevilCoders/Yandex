PY2_PROGRAM()

OWNER(g:aapi)

PEERDIR(
    contrib/python/patched/subvertpy
    contrib/python/futures
    aapi/lib/py_common
    aapi/lib/py_helpers
    aapi/lib/convert
)

PY_SRCS(
    __main__.py
)

END()
