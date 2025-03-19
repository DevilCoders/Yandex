LIBRARY()

OWNER(g:base)

SRCS(
    fat.cpp
    hitwriter.cpp
    indexfile.cpp
    indexfileimpl.h
    indexreader.h
    indexstorage.cpp
    indexstoragefactory.cpp
    indexutil.cpp
    indexwriter.cpp
    fatwriter.cpp
    memoryportion.h
    oldindexfile.h
    oldindexfileimpl.h
    rdkeyit.cpp
    searchfile.cpp
    seqreader.cpp
    subindexwriter.cpp
)

GENERATE_ENUM_SERIALIZATION(indexutil.h)

PEERDIR(
    kernel/keyinv/hitlist
    kernel/keyinv/invkeypos
    kernel/search_types
    library/cpp/wordpos
    library/cpp/yappy
    tools/enum_parser/enum_serialization_runtime
)

END()
