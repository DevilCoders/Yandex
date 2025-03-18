LIBRARY()

OWNER(g:antirobot)

PEERDIR(
    library/cpp/openssl/crypto
    library/cpp/string_utils/base64
    contrib/libs/taocrypt
)

SRCS(
    fuid.cpp
)

END()
