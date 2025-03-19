LIBRARY()

OWNER(
    my34
    g:snippets
)

SRCS(
    markers.cpp
    markers.h
)

GENERATE_ENUM_SERIALIZATION(markers.h)

PEERDIR(
    library/cpp/string_utils/base64
)

END()
