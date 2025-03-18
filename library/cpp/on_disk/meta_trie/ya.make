LIBRARY()

OWNER(velavokr)

SRCS(
    metatrie.cpp
    index_digest.cpp
)

PEERDIR(
    library/cpp/binsaver
    library/cpp/containers/comptrie
    library/cpp/on_disk/codec_trie
    library/cpp/deprecated/solartrie
)

END()
