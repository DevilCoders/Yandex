LIBRARY()

OWNER(sashateh)

PEERDIR(
    contrib/libs/re2
    kernel/mango/common
    kernel/urlid
    library/cpp/cgiparam
    library/cpp/html/entity
    library/cpp/html/face
    library/cpp/html/html5
    library/cpp/html/zoneconf
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/uri
)

SRCS(
    normalize_url.cpp
    info.cpp
    findurl.cpp
    url_canonizer.cpp
)

END()
