OWNER(g:facts)

LIBRARY()

SRCS(
    dictionary.cpp
    features_calculator.cpp
    preprocessor.cpp
    weighter.cpp
    word_embedding.cpp
)

PEERDIR(
    kernel/dssm_applier/utils
    kernel/facts/factors_info
    kernel/facts/features_calculator/embeddings
    kernel/lemmer
    library/cpp/scheme
    quality/trailer/trailer_common/mmap_preloaded
)

GENERATE_ENUM_SERIALIZATION(features_calculator.h)

RESOURCE(word_embedding.config word_embedding_config)

END()
