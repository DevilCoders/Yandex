LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    contrib/libs/jwt-cpp

    kernel/common_server/abstract
    kernel/common_server/library/openssl
    kernel/common_server/ciphers/kms
    kernel/common_server/ciphers/pool
    kernel/common_server/ciphers/reserve
)

SRCS(
    abstract.cpp
    config.cpp
    GLOBAL external.cpp
    GLOBAL aes.cpp
    GLOBAL chain.cpp
    GLOBAL rsa.cpp
)

END()
