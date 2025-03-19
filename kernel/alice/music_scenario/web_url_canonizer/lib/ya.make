LIBRARY()

OWNER(
    hommforever
    g:alice_quality
)

SRCS(
    web_url_canonizer.cpp
)

PEERDIR(
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
)

END()
