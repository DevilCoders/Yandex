OWNER(
    kartynnik  # inherited from kolesov93@
)

LIBRARY()

PEERDIR(
    kernel/indexer/face
    kernel/xpathmarker/entities
    kernel/xpathmarker/utils
    library/cpp/json
)

SRCS(
    archivewriter.cpp
    jsonwriter.cpp
)

END()
