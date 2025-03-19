OWNER(grechnik)

LIBRARY()

PEERDIR(
    kernel/dups/banned_info/protos
    kernel/dups/banned_info
    kernel/dups/proto
    contrib/libs/muparser
    kernel/searchlog
    kernel/urlnorm
    library/cpp/charset
    library/cpp/regex/pcre
    contrib/libs/re2
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
)

SRCS(
    banned_info.cpp
    banned_info_reader.cpp
    check.cpp
)

GENERATE_ENUM_SERIALIZATION(check.h)

END()
