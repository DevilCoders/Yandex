LIBRARY()

OWNER(alexbykov)

PEERDIR(
    kernel/qtree/compressor
    library/cpp/string_utils/base64
)

IF (CLANG)
    CFLAGS(
        -Wno-unused-function
    )
ENDIF()

SRCS(
    saas_trie.proto
    trie_key.cpp
)

GENERATE_ENUM_SERIALIZATION(trie_key.h)

END()
