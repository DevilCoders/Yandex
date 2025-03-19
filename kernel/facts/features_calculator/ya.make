LIBRARY()

OWNER(g:facts)

SRCS(
    calculator_data.cpp
    calculator_data_mr.cpp
    calculator_config.cpp
    query_features.cpp
    serp_features.cpp
)

PEERDIR(
    kernel/dssm_applier/nn_applier/lib
    kernel/facts/common
    kernel/facts/common_features
    kernel/facts/dist_between_words
    kernel/facts/dssm_applier
    kernel/facts/features_calculator/embeddings
    kernel/facts/features_calculator/idl
    kernel/facts/edit_distance_features
    kernel/facts/word_difference
    kernel/facts/word_embedding
    kernel/normalize_by_lemmas
    ml/neocortex/neocortex_lib
    quality/trailer/trailer_common
    ysite/yandex/pure
    library/cpp/string_utils/url
)

END()
