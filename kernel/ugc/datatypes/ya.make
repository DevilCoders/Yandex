OWNER(g:ugc)

LIBRARY()

PEERDIR(
    library/cpp/scheme
    library/cpp/string_utils/base64
    library/cpp/unicode/punycode
)

SRCS(
    objectid.cpp
    props.cpp
    update.cpp
    userid.cpp
)

END()
