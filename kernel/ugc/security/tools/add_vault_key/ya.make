OWNER(
    g:ugc
)

PY2_PROGRAM()

PEERDIR(
    contrib/python/paramiko
    contrib/python/requests
)

PY_SRCS(
    __main__.py
    generate.py
    oauth.py
    vault.py
)

END()
