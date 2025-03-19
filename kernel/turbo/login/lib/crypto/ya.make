OWNER(g:turbo)

RECURSE_FOR_TESTS(ut)

LIBRARY()

SRCS(
    crypto.cpp
    fingerprint.cpp
    key_gen.cpp
    key_provider.cpp
    sign.cpp
)

PEERDIR(
    contrib/libs/libsodium
    contrib/libs/openssl
    kernel/turbo/login/lib/crypto/proto
    library/cpp/cgiparam
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
    library/cpp/string_utils/quote
    library/cpp/json
)


END()
