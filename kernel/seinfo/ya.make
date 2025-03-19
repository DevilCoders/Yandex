LIBRARY()

OWNER(
    akhropov
    smikler
)

PEERDIR(
    contrib/libs/openssl
    kernel/search_query
    library/cpp/binsaver
    library/cpp/cgiparam
    library/cpp/charset
    library/cpp/containers/dictionary
    library/cpp/deprecated/split
    library/cpp/json
    library/cpp/openssl/holders
    library/cpp/regex/libregex
    library/cpp/regex/pcre
    library/cpp/regex/pire
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/xml/document
)

SRCS(
    non_common.cpp
    services.cpp
    seinfo.cpp
    enums_impl.cpp
    regexps_impl.cpp
    regexps.rl6
)

GENERATE_ENUM_SERIALIZATION(enums.h)

GENERATE_ENUM_SERIALIZATION(services.h)

END()
