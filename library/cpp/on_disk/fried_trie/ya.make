LIBRARY()

OWNER(
    anelyubin
    melkov
)

SRCS(
    trie.cpp
    trie_builder.h
    trie_common.cpp
    trie_enumerated.cpp
    trie_enumerated_trackers.h
    trie_file.cpp
)

PEERDIR(
    library/cpp/deprecated/datafile
    library/cpp/microbdb
    library/cpp/remmap
    library/cpp/logger/global
)

END()
