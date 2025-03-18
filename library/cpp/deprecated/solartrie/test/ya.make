PROGRAM(solartrie_test)

OWNER(velavokr)

ALLOCATOR(LF)

# SET_APPEND(LIBS -lprofiler)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/on_disk/codec_trie
    library/cpp/deprecated/solartrie
    library/cpp/string_utils/base64
)

SRCS(
    solartrie_test.cpp
)

END()
