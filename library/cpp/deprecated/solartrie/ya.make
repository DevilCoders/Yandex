LIBRARY()

OWNER(velavokr)

PEERDIR(
    library/cpp/codecs
    library/cpp/binsaver
    library/cpp/containers/comptrie
    library/cpp/packers
    library/cpp/deprecated/solartrie/indexed_region
    library/cpp/containers/paged_vector
)

SRCS(
    solartrie.h
    trie_conf.cpp
    trie_private.cpp
    triebuilder.h
    triebuilder_private.cpp
)

END()
