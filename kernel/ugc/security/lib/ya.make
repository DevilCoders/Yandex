OWNER(
    g:ugc
    deep
)

LIBRARY()

SRCS(
    hash_sign.cpp
    pseudonym.cpp
    record_identifier.cpp
    secret_manager.cpp
    token.cpp
)

PEERDIR(
    kernel/ugc/runtime
    kernel/ugc/security/lib/internal
    kernel/ugc/security/proto
    library/cpp/string_utils/base64
)

END()
