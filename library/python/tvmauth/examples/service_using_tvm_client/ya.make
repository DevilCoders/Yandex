PY23_LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    contrib/python/requests
    library/python/tvmauth
)

PY_SRCS(
    __init__.py
    serv.py
)

END()
