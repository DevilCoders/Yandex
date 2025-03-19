UNITTEST_FOR(kernel/user_history/lib)

OWNER(
    avatar
    g:search-pers
)

SRCS(
    embedding_tools_ut.cpp
    fading_embedding_tools_ut.cpp
    merge_ut.cpp
)

PEERDIR(
    kernel/embeddings_info/lib
    kernel/user_history
    kernel/user_history/proto
    library/cpp/dot_product
    library/cpp/protobuf/util
    library/cpp/testing/unittest
)

END()
