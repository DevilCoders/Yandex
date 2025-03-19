LIBRARY()

OWNER(
    smikler
    dpaveldev
    g:yabs-small
)

SRCS(
    urlhashval.rl
    host.h
    host.cpp
    normalize.h
    normalize.cpp
    urlnorm.h
    urlnorm.cpp
    validate.h
    validate.cpp
)

PEERDIR(
    library/cpp/cgiparam
    library/cpp/digest/md5
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/uri
)

END()

RECURSE_FOR_TESTS(ut)
