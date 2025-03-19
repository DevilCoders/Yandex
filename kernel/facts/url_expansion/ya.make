LIBRARY()

OWNER(
    g:facts
)

SRCS(
    url_expansion.cpp
)

PEERDIR(
    quality/functionality/turbo/urls_lib/cpp/lib
    library/cpp/cgiparam
    library/cpp/string_utils/url
)

END()
