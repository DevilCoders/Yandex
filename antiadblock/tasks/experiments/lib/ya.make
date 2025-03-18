PY2_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    common.py
)

PEERDIR(
    contrib/python/requests
    library/python/tvmauth
)

END()
