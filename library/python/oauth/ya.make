PY23_LIBRARY()

OWNER(pg)

PEERDIR(
    contrib/python/requests
    library/python/ssh_sign
)

PY_SRCS(__init__.py)

END()
