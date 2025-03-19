LIBRARY()

OWNER(
    g:clickdaemon
    pg
    avitella
)

PEERDIR(
    contrib/libs/openssl
    library/cpp/cgiparam
    library/cpp/digest/md5
    library/cpp/json
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
)

SRCS(
    keyholder.cpp
    signurl.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
