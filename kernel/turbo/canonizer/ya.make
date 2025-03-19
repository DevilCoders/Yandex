LIBRARY()

OWNER(
    g:turbo
)

SRCS(
    canonizer.cpp
)

PEERDIR(
    library/cpp/string_utils/url
    library/cpp/string_utils/quote
    library/cpp/uri
)

END()
