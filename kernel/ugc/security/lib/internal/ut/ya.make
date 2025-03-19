OWNER(
    g:ugc
    deep
)

UNITTEST_FOR(kernel/ugc/security/lib/internal)

SRCS(
    crypto_ut.cpp
)

PEERDIR(
    kernel/ugc/security/proto
)

END()
