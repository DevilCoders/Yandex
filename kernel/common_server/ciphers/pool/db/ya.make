LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/openssl
    kernel/common_server/library/storage
    kernel/common_server/abstract
    kernel/common_server/api/history
    kernel/common_server/ciphers/pool/abstract
)

SRCS(
    dek.cpp
    GLOBAL pool.cpp
)

END()
