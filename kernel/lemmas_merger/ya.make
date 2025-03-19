LIBRARY()

OWNER(
    alex-sh
    mvel
)

SRCS(
    words_trie.h
    lemmas_merger.h
    lemmas_merger.cpp
)

PEERDIR(
    library/cpp/binsaver
    library/cpp/disjoint_sets
    library/cpp/containers/comptrie
    library/cpp/langmask
    library/cpp/tokenizer
    library/cpp/token
)

END()
