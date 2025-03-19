LIBRARY()

OWNER(
    g:base
    g:factordev
    mvel
)

SRCS(
    derive_matrixnet.cpp
    rank_models_factory.cpp
    rank_models_info.cpp
    relev_fml.cpp
    relev_multiple.cpp
)

PEERDIR(
    kernel/bundle
    kernel/catboost
    kernel/country_data
    kernel/dssm_applier
    kernel/dssm_applier/optimized_model
    kernel/factor_slices
    kernel/factor_storage
    kernel/matrixnet
    kernel/relevfml/models_archive
    kernel/search_types
    library/cpp/archive
    library/cpp/string_utils/ascii_encode
    library/cpp/vowpalwabbit
    kernel/dssm_applier/query_word_title
)

END()
