PROGRAM()

OWNER(g:base)

SRCS(
    datagen.cpp
)

PEERDIR(
    kernel/indexer/lexical_decomposition
    library/cpp/charset
    library/cpp/getopt
)

END()

NEED_CHECK()
