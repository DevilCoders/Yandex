LIBRARY()

OWNER(
    olegator
    tagrimar
)

SRCS(
    boosting_compression.cpp
    doc_embedding.cpp
    dssm_url_prediction.cpp
    embedding_generator_adapter.cpp
    shows_prediction.cpp
)

PEERDIR(
    kernel/dssm_applier/decompression
    kernel/dssm_applier/nn_applier/lib
    kernel/dssm_applier/utils
    library/cpp/charset
    library/cpp/dot_product
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(doc_embedding.h)

END()
