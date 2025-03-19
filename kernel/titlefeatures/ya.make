OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    titlefeatures.cpp
)

PEERDIR(
    kernel/idf
    kernel/indexer/direct_text
    kernel/indexer/face
    kernel/search_types
    library/cpp/on_disk/2d_array
    ysite/relevance_tools
    ysite/yandex/pure
)

END()
