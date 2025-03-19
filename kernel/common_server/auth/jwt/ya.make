LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/auth/common
    kernel/common_server/library/openssl
    kernel/common_server/util
)

SRCS(
    jwk.cpp
    GLOBAL jwt.cpp
)

END()
