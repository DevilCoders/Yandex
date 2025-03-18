OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    query_marker.cpp
    query_marker_aho.h
    query_marker_trie.h
)

PEERDIR(
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/on_disk/aho_corasick
)

END()
