OWNER(
    g:ugc
    deep
)

UNITTEST_FOR(kernel/ugc/security/lib)

SRCS(
    pseudonym_ut.cpp
    record_identifier_ut.cpp
    secret_manager_ut.cpp
    token_ut.cpp
)

PEERDIR(
    kernel/ugc/security/proto
)

END()
