OWNER(g:turbo)

PY2_LIBRARY()


PEERDIR(
    contrib/libs/libsodium
    kernel/turbo/login/lib/crypto
)

PY_SRCS(
    crypto.pyx
)

END()

