OWNER(
    kartynnik  # inherited from kolesov93@
)

LIBRARY()

PEERDIR(
    contrib/libs/libxml
    kernel/indexer/faceproc
    kernel/xpathmarker/configs
    kernel/xpathmarker/entities
    kernel/xpathmarker/fetchers
    kernel/xpathmarker/utils
    kernel/xpathmarker/xmlwalk
    kernel/xpathmarker/zonewriters
    library/cpp/html/face
    library/cpp/html/norm
    library/cpp/html/storage
    library/cpp/html/tree
    library/cpp/json
    library/cpp/regex/libregex
    library/cpp/regex/pire
    library/cpp/xml/doc
    util/draft
)

SRCS(
    xpathmarker.cpp
)

END()
