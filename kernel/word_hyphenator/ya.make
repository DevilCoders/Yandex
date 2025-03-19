LIBRARY()

OWNER(
    alspi
)

SRCS(
    word_hyphenator.cpp
)

RESOURCE(
    word_parts_trie /word_parts_trie
)

PEERDIR(
    library/cpp/containers/comptrie
)

END()
