LIBRARY()

OWNER(g:base)

PEERDIR(
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/deprecated/split
    library/cpp/langs
    library/cpp/on_disk/aho_corasick
    library/cpp/on_disk/chunks
)

SRCS(
    lexical_decomposition_algo.cpp
    token_lexical_splitter.cpp
    vocabulary_builder.cpp
)

END()

NEED_CHECK()
