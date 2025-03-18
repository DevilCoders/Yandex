LIBRARY()

OWNER(velavokr)

SRCS(
    codectrie.cpp
    trie_conf.cpp
    triebuilder.h
)

PEERDIR(
    library/cpp/binsaver
    library/cpp/codecs
    library/cpp/containers/comptrie
    library/cpp/containers/paged_vector
    library/cpp/deprecated/accessors
    library/cpp/packers
)

END()
