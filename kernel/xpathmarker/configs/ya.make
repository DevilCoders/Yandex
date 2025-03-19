OWNER(
    kartynnik  # inherited from kolesov93@
)

LIBRARY()

PEERDIR(
    contrib/libs/libxml
    kernel/xpathmarker/entities
    kernel/xpathmarker/fetchers
    kernel/xpathmarker/utils
    kernel/xpathmarker/xmlwalk
    library/cpp/html/tree
    library/cpp/json
    library/cpp/xml/doc
)

SRCS(
    attribute_config.cpp
    zone_config.cpp
)

END()
