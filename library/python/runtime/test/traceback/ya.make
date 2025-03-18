PY2_PROGRAM()

OWNER(borman)

PEERDIR(
    contrib/python/ipython
)

PY_SRCS(
    __main__.py
    crash.py
    mod/__init__.py
)

END()
