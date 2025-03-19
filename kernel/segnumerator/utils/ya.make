LIBRARY()

OWNER(velavokr)

SRCS(
    url_functions.h
    event_storer.cpp
)

PEERDIR(
    contrib/libs/libidn
    kernel/hosts/owner
    kernel/segmentator/structs
    kernel/urlnorm
    library/cpp/charset
    library/cpp/html/face
    library/cpp/html/spec
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/token
)

END()
