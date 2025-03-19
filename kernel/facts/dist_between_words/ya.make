LIBRARY()

OWNER(g:facts)

SRCS(
    calc_factors.cpp
    levenstein_dist_with_bigrams.cpp
    levenstein_word_dist.cpp
    trie_data.cpp
)

PEERDIR(
    kernel/facts/factors_info
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/packers
    library/cpp/string_utils/levenshtein_diff
    library/cpp/tokenizer
    util/draft
)

END()
