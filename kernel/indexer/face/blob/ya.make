LIBRARY()

OWNER(shuster)

PEERDIR(
    kernel/indexer/direct_text
    kernel/indexer/face
    kernel/search_types
    library/cpp/charset
    library/cpp/packedtypes
)

SRCS(
    directtext.proto
    markup.proto
    datacontainer.cpp
    directzoneinserter.cpp
    markup.cpp
    read.cpp
    write.cpp
)

END()
