LIBRARY()

OWNER(g:facts)

SRCS(
    cross_model.cpp
    query_model.cpp
    query_tokens.cpp
)

PEERDIR(
    kernel/ethos/lib/text_classifier
    kernel/facts/features_calculator/analyzed_word
    kernel/lemmer/core
    library/cpp/tokenizer
    search/web/util/lemmer_cache
)

END()
