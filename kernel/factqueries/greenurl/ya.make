LIBRARY()

OWNER(
    bogolubsky
    g:facts
)

SRCS(
    greenurl.cpp
)

PEERDIR(
    kernel/url_tools
    library/cpp/scheme
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/uri
)

END()

RECURSE_FOR_TESTS(ut)
