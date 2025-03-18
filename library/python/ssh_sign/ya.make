PY23_LIBRARY()

OWNER(
    pg
    g:yatool
)

PEERDIR(
    library/python/openssl
    contrib/python/paramiko
    contrib/python/six
)

PY_SRCS(__init__.py)

END()
