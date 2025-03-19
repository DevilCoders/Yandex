OWNER(
    g:ugc
    deep
)

LIBRARY()

SRCS(
    crypto.cpp
    random.cpp
)

PEERDIR(
    contrib/libs/openssl
    kernel/ugc/security/proto
)

END()
