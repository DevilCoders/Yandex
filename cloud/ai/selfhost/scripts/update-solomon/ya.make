PY3_PROGRAM()

OWNER(g:cloud-ai)

PY_SRCS(
    MAIN update-solomon.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/PyJWT
)

END()
