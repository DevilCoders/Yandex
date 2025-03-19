OWNER(
    kartynnik # inherited from kolesov93@
)

LIBRARY()

PEERDIR(
    contrib/libs/libxml
    kernel/dater
    kernel/dater/convert_old
    kernel/xpathmarker/entities
    kernel/xpathmarker/utils
    kernel/xpathmarker/xmlwalk
    library/cpp/json
    library/cpp/xml/doc
)

SRCS(
    universalfetcher.cpp
    constants.cpp
)

END()
