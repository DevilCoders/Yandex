LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    contrib/libs/openssl
    library/cpp/logger/global
    library/cpp/openssl/init
    library/cpp/string_utils/base64
    kernel/common_server/library/logging
)

SRCS(
    aes.cpp
    oio.cpp
    rsa.cpp
    types.cpp
)

END()
