LIBRARY()

OWNER(g:morphology)

PEERDIR(
    library/cpp/archive
    library/cpp/containers/comptrie
)

ARCHIVE_ASM(
    NAME TrieBin
    DONTCOMPRESS
    trie.bin
)

SRCS(
    superlemmer.cpp
)

GENERATE_ENUM_SERIALIZATION(superlemmer_version.h)

END()
