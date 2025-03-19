UNITTEST_FOR(kernel/nn_ops)

OWNER(olegator)

PEERDIR(
    library/cpp/dot_product
    kernel/nn_ops
)

SRCS(
    doc_embedding_ut.cpp
    boosting_compression_ut.cpp
)

END()
