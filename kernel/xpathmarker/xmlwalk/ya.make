OWNER(
    kartynnik  # inherited from kolesov93@
)

LIBRARY()

PEERDIR(
    contrib/libs/libxml
    library/cpp/html/entity
    library/cpp/json
    library/cpp/regex/libregex
    library/cpp/xml/doc
)

SRCS(
    sanitize.cpp
    utils.cpp
    xmlwalk.cpp
)

END()
