LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/lemmer/alpha
    library/cpp/containers/comptrie
    library/cpp/containers/comptrie/loader
    library/cpp/deprecated/split
    library/cpp/tokenizer
)

SRCS(
    capital.cpp
)

ARCHIVE(
    NAME capital.inc
    capital_blacklist.trie
    capital_special.trie
)

END()
