PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(0.72)

PEERDIR(
    contrib/python/six
    library/python/yenv
    contrib/python/retrying
)

PY_SRCS(
    TOP_LEVEL
    blackbox.py
)

END()
