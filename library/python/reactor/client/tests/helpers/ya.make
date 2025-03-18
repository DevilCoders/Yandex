PY23_LIBRARY()

OWNER(g:reactor)

PY_SRCS(
    __init__.py
    utils.py
)

PEERDIR(
    library/python/reactor/client
)

END()
