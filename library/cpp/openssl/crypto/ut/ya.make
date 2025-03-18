UNITTEST_FOR(library/cpp/openssl/crypto)

OWNER(g:util anelyubin pg)

PEERDIR(
    contrib/libs/openssl
)

SRCS(
    rsa_ut.cpp
    sha_ut.cpp
)

END()
