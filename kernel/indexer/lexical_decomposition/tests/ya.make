UNITTEST()

OWNER(g:base)

PEERDIR(
    kernel/indexer/lexical_decomposition
    library/cpp/charset
    library/cpp/deprecated/split
)

SRCS(
    lexical_decomposition_ut.cpp
)

END()

NEED_CHECK()
