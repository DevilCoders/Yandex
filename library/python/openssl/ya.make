PY23_LIBRARY()

OWNER(
    g:yatool
    dmitko
    pg
)

NO_WSHADOW()

PEERDIR(
    contrib/libs/openssl
)

PY_SRCS(
    TOP_LEVEL
    openssl.pyx
)

END()
