UNITTEST()

OWNER(g:turbo)

PEERDIR(
    kernel/turbo/login/lib/crypto
)

SRCS(
    crypto_ut.cpp
    sign_ut.cpp
)

SRCDIR(
    kernel/turbo/login/lib/crypto
)

END()
