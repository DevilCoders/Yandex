PY23_LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    library/python/auth_client_parser
)

PY_SRCS(
    __init__.py
    cookie.py
    token.py
)

END()
